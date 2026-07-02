#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <inttypes.h>

#include "application.h"

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

StatusLED statusIndicator;

const int SENSOR_BLUE_PIN = 9;
const int SENSOR_YELLOW_PIN = 10;

// Combined hardware debounce and physical sensor lockout window
const uint64_t LOCKOUT_WINDOW_US = 50000;
const int EVENT_QUEUE_LENGTH = 100;

// --- Event Types ---
typedef enum { EVT_ASSERTED, EVT_RELEASED } RawEdge_t;

typedef enum {
  ACTION_NONE,
  ACTION_TRIGGER,
} ActionType_t;

struct SensorEvent {
  int pin;
  RawEdge_t edge;
  ActionType_t action = ACTION_NONE;
  uint64_t tsf_time = 0;        // LOCKED: Hard leading-edge network baseline (0 if offline)
  uint64_t processor_time = 0;  // LOCKED: Hard leading-edge processor baseline (microseconds)
};

static QueueHandle_t xRawQueue = NULL;
static QueueHandle_t xActionQueue = NULL;
static portMUX_TYPE fsm_mux = portMUX_INITIALIZER_UNLOCKED;

// --- Input Source FSM & Channel Tracking Struct ---
typedef enum { STATE_RELEASED, STATE_ASSERTED } SourceState_t;

struct SourceParams {
  int pin;
  SourceState_t fsm_state;
  int active_level;
  SensorEvent leading_edge;        // Persistent state baseline tracking
  uint64_t last_valid_press_time;  // Tracks localized lockout state per channel safely
};

// Initialized with last_valid_press_time = 0
static SourceParams sensor_blue = {SENSOR_BLUE_PIN, STATE_RELEASED, LOW, {}, 0};
static SourceParams sensor_yellow = {SENSOR_YELLOW_PIN, STATE_RELEASED, LOW, {}, 0};

// Registration array for safe, safe arbitrary channel lookups
static SourceParams *active_channels[] = {&sensor_blue, &sensor_yellow};
const int NUM_CHANNELS = sizeof(active_channels) / sizeof(active_channels[0]);

/**
 * @brief Maps a GPIO pin number to its parameters configuration structure safely.
 */
static SourceParams *get_source_by_pin(int pin) {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (active_channels[i]->pin == pin) {
      return active_channels[i];
    }
  }
  return NULL;
}

/**
 * @brief Query whether a sensor input is currently active. Safe to call from any task.
 */
bool is_input_active(int pin) {
  SourceParams *p = get_source_by_pin(pin);
  if (!p) {
    return false;
  }
  portENTER_CRITICAL(&fsm_mux);
  bool pressed = (p->fsm_state == STATE_ASSERTED);
  portEXIT_CRITICAL(&fsm_mux);
  return pressed;
}

/**
 * @brief Fast hardware ISR. Intercepts edge and clocks event immediately.
 */
static void IRAM_ATTR input_isr(void *arg) {
  // Grab the hardware time IMMEDIATELY on entry
  uint64_t now = esp_timer_get_time();
  SourceParams *p = (SourceParams *)arg;

  // Handle split register mapping for ESP32 GPIO ranges safely
  // This executes the exact same underlying raw register assembly in 1 line
  bool pin_high = (gpio_get_level((gpio_num_t)p->pin) != 0);

  SensorEvent msg;
  msg.pin = p->pin;
  msg.edge = (pin_high == (p->active_level == HIGH)) ? EVT_ASSERTED : EVT_RELEASED;
  msg.processor_time = now;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(xRawQueue, &msg, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

/**
 * @brief Purely event-driven Edge-Broker.
 * Blocks infinitely until the hardware ISR delivers a transition event.
 * Enforces the lockout window chronologically, completely bypassing polling loops.
 */
static void input_broker_task(void *pvParameters) {
  SensorEvent msg;

  for (;;) {
    // FIXED: Blocks indefinitely with portMAX_DELAY. Uses 0% CPU unless an event arrives.
    if (xQueueReceive(xRawQueue, &msg, portMAX_DELAY) == pdTRUE) {
      SourceParams *p = get_source_by_pin(msg.pin);
      if (!p)
        continue;

      if (msg.edge == EVT_ASSERTED) {
        // Enforce the lockout/debounce window on the incoming leading edge timestamp
        if (msg.processor_time - p->last_valid_press_time >= LOCKOUT_WINDOW_US) {
          p->last_valid_press_time = msg.processor_time;

          // Reconstruct Network TSF Timing baseline against the original raw ISR mark
          uint64_t current_proc_time = esp_timer_get_time();
          uint64_t exact_leading_edge_tsf = 0;

          wifi_mode_t mode;
          if (esp_wifi_get_mode(&mode) == ESP_OK && mode != WIFI_MODE_NULL) {
            uint64_t current_tsf_time = esp_wifi_get_tsf_time(WIFI_IF_STA);
            uint64_t task_latency = current_proc_time - msg.processor_time;
            exact_leading_edge_tsf = current_tsf_time - task_latency;
          }

          p->leading_edge.pin = p->pin;
          p->leading_edge.edge = EVT_ASSERTED;
          p->leading_edge.action = ACTION_TRIGGER;
          p->leading_edge.tsf_time = exact_leading_edge_tsf;
          p->leading_edge.processor_time = msg.processor_time;

          xQueueSend(xActionQueue, &(p->leading_edge), 0);

          p->fsm_state = STATE_ASSERTED;
        }
      } else if (msg.edge == EVT_RELEASED) {
        // Trailing edges update state flags instantly without needing tracking loops
        p->fsm_state = STATE_RELEASED;
      }
    }
  }
}

/**
 * @brief Receives fully-validated leading-edge events and logs them.
 * No validation math occurs here; it acts as a pure networking serialization layer.
 */
static void action_handler_task(void *pvParameters) {
  SensorEvent evt;

  for (;;) {
    if (xQueueReceive(xActionQueue, &evt, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (evt.action == ACTION_TRIGGER) {
      // Logic is guaranteed valid by the entry filter inside the broker task
      Serial.printf("LOGGED TRIGGER | Pin: %d | TSF: %" PRIu64 " | Proc: %" PRIu64 "\n", evt.pin, evt.tsf_time, evt.processor_time);

      // Wi-Fi transmit logic can safely block here without hindering the broker thread
    }
  }
}

void app_setup() {
  statusIndicator.begin();
  delay(2000);
  Serial.begin(SERIAL_BAUD);
  for (int i = 0; i < 300 && !Serial; i++) {
    delay(10);
  }
  statusIndicator.turnOn();

  char *board_name = (char *)get_board_name();
  Serial.printf("Board: %s\n", board_name);

  // Allocating uniform message space across both queues
  xRawQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(SensorEvent));
  xActionQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(SensorEvent));

  if (xRawQueue != NULL && xActionQueue != NULL) {
    pinMode(SENSOR_BLUE_PIN, INPUT_PULLUP);
    pinMode(SENSOR_YELLOW_PIN, INPUT_PULLUP);

    attachInterruptArg(digitalPinToInterrupt(SENSOR_BLUE_PIN), input_isr, &sensor_blue, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(SENSOR_YELLOW_PIN), input_isr, &sensor_yellow, CHANGE);

    xTaskCreate(input_broker_task, "InputBroker", 3072, NULL, 3, NULL);  // Slipped priority up slightly for precision
    xTaskCreate(action_handler_task, "ActionHandler", 2048, NULL, 2, NULL);
  }

  Serial.println("System running.");
}

void app_loop() {
  if (is_input_active(SENSOR_BLUE_PIN)) {
    statusIndicator.turnOn();
  }
  if (is_input_active(SENSOR_YELLOW_PIN)) {
    statusIndicator.turnOff();
  }
  delay(100);
}