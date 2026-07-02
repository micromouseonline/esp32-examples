#include <Arduino.h>
#include <NimBLEDevice.h>

#include "../common/board-id.h"
#include "../common/esp32_info.h"

const int LED_BLUE = 1;
const int LED_YELLOW = 2;

/**
 * @brief Parameters passed to each blink task.
 */
struct BlinkParams {
  int pin;
  uint32_t interval_ms;
};

static BlinkParams blink_blue = {LED_BLUE, 431};
static BlinkParams blink_yellow = {LED_YELLOW, 359};

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
volatile bool deviceConnected = false;

// Create a queue that holds float values
QueueHandle_t temperature_queue = xQueueCreate(10, sizeof(float));

/**
 * @brief FreeRTOS task: toggles a single LED at a fixed interval.
 * @param pvParameters  Pointer to a BlinkParams struct.
 */
static void blink_task(void *pvParameters) {
  BlinkParams *p = (BlinkParams *)pvParameters;
  pinMode(p->pin, OUTPUT);
  for (;;) {
    digitalWrite(p->pin, !digitalRead(p->pin));
    vTaskDelay(pdMS_TO_TICKS(p->interval_ms));
  }
}

/**
 * @brief NEW FreeRTOS task: Sends a BLE message exactly once per second
 */
static void ble_sender_task(void *pvParameters) {
  for (;;) {
    if (deviceConnected) {
      // BLE Task pulls data (blocks automatically until data arrives):
      float received_temp;
      if (xQueueReceive(temperature_queue, &received_temp, portMAX_DELAY) == pdPASS) {
        // Send via BLE...
        // Generate the message string inside the task loop safely
        String msg = "Temperature: " + String(received_temp, 2) + " °C Time: " + String(millis() / 1000) + "\n";

        // Send the data via NimBLE
        pTxCharacteristic->setValue((uint8_t *)msg.c_str(), msg.length());
        pTxCharacteristic->notify();
      }
    }

    // Delay for exactly 1 second (1000 ms) before running again
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Nordic UART Service UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Device disconnected. Restarting advertising...");
    pServer->getAdvertising()->start();  // Automatically restart advertising
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.print("Received Value: ");
      for (size_t i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      Serial.println();
    }
  }
};

void app_setup() {
  delay(2000);  // Give serial monitor time to connect
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    delay(10);  // Wait for Serial to be ready
  }

  // Initialize NimBLE
  char *board_name = (char *)get_board_name();
  Serial.printf("Board: %s\n", board_name);
  BLEDevice::init(board_name);
  // INCREASE MTU SIZE HERE (Request maximum size)
  BLEDevice::setMTU(517);

  // Create Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX Characteristic (for sending data to phone)
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);

  // Create RX Characteristic (for receiving data from phone)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start service & advertising
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  xTaskCreate(blink_task, "blink_blue", 2048, &blink_blue, 1, NULL);
  xTaskCreate(blink_task, "blink_yellow", 2048, &blink_yellow, 1, NULL);

  // Create our new BLE Transmission task (Priority 1)
  xTaskCreate(ble_sender_task, "ble_sender", 3072, NULL, 1, NULL);

  Serial.println("BLE Serial Server is running!");
}

void app_loop() {
  // Sensor Task pushes data:
  float new_temp = millis() / 1000.0f;
  xQueueSend(temperature_queue, &new_temp, pdMS_TO_TICKS(10));
  vTaskDelay(pdMS_TO_TICKS(100));
}