# Cerberus: align input architecture to design doc (skeleton only)

## Context
Cerberus is a demo/tester, not the race timer itself. We want the *input
layer* of DESIGN-REQUIREMENT.md (physical + touch buttons -> event queue ->
state dispatch) built for real, wired to drive the existing font browser
(`show_fonts_structured`) instead of race-timing logic. HTTP, logging, SD,
NVS, RACING/MAINTENANCE stay out of scope.

Today's gaps vs the design:
- `show_fonts_structured()` is a self-contained blocking `while(true)` loop
  that owns its own touch polling and never returns -- can't be driven by
  external events, and starves `app_loop()` on the board it runs on.
- Button polling (GPIO + touch) happens inline in `app_loop()`, not a
  separate Core-1 task like the design's "Local Input Polling Task".
- Physical-vs-touch input is hardwired per `BOARD_*` ifdef rather than by
  capability, so a future "CYD + NeoKey" target has nowhere to plug in.

Three boards to build+test against once Step 3 lands: `cerberus-m5-core`
(physical buttons A/B/C, no touch), `cerberus-esp32-s3-cyd-touch-freenove`
(capacitive touch, I2C, no physical buttons), and `cerberus-cyd2usb-diymalls-ili9341`
(resistive touch, XPT2046 over SPI -- shares the display bus, unlike the
other two). A fourth board (CYD + 4-button NeoKey I2C expander, driver code
already in hand) slots in at **Step 10** without touching earlier steps.

## Step-by-step (each step builds + is manually testable on the boards available so far)

### Step 1 -- Capability flags in `config.h` -- **DONE**
Add `HAS_TOUCH_INPUT` / `HAS_GPIO_BUTTONS` / `HAS_NEOKEY_BUTTONS` per board
(M5: touch=0,gpio=1,neokey=0; CYD: touch=1,gpio=0,neokey=0). Swap
`gpio-buttons.h`'s `#if defined(BOARD_M5_CORE)` guard for
`#if HAS_GPIO_BUTTONS`. No behavior change.
- **Build:** both envs (M5, CYD touch).
- **Test:** M5 physical buttons still log ARM/START/GOAL/RESET as before;
  CYD unaffected.
- *Status: both envs built successfully. Awaiting your on-hardware
  confirmation that M5 buttons still behave as before.*

### Step 2 -- Extract touch handling into `touch-buttons.h` -- **DONE**
Move `init_ui`/`loop_ui` out of `application.cpp` into new
`touch-buttons.h`: `init_touch_buttons(LGFX&)`, `poll_touch_buttons()`
(still does press-flash drawing, still called synchronously from
`app_loop()`, same as today). Guard on `HAS_TOUCH_INPUT`. Pure refactor.
- **Build:** both envs.
- **Test:** CYD touch buttons behave identically (press flash + serial log).
  M5 unaffected (flag off).
- *Status: both envs built successfully. Note: on M5 the on-screen button bar
  is no longer drawn at all (previously `init_ui` drew it unconditionally,
  even though M5 has no touch to press it) -- a minor, arguably-correct
  behavior change from gating on `HAS_TOUCH_INPUT`. Flag if you want the bar
  back on M5 as a non-interactive display. Awaiting your on-hardware
  confirmation of CYD touch behavior.*

### Step 3 -- Add third board: CYD2USB_DIYMALLS_ILI9341 (resistive XPT2046 touch, SPI) -- **DONE**
New board target reusing Step 1/2 code unchanged: LovyanGFX's `getTouch()`
abstracts XPT2046 the same as FT6336U, so `touch-buttons.h` needs no code
change, only a new hardware profile.
- ~~Add `BOARD_CYD2USB_DIYMALLS_ILI9341` to `common/board-config.h`/`board-id.h` alongside the
  existing two.~~ Turned out unnecessary: those two files have no per-board
  branching (board macros are only consumed in `config.h`, `display.h`,
  `gpio-buttons.h`, `application.cpp`). Corrected here after checking.
- New `base-boards.ini` section `[base_cyd2usb_diymalls_ili9341]` (plain ESP32, `esp32dev`,
  4MB flash -- commonly-documented spec for this board family) defining
  `-D BOARD_CYD2USB_DIYMALLS_ILI9341`, plus `[env:cerberus-cyd2usb-diymalls-ili9341]` in `platformio.ini`.
- New `config.h` `#elif defined(BOARD_CYD2USB_DIYMALLS_ILI9341)` block: `DISPLAY_TOUCH_XPT2046`,
  panel pins matching the classic ESP32-2432S028R pinout, and corrected
  touch pin naming (`PIN_TOUCH_CS`/`PIN_TOUCH_IRQ`, SPI-style) instead of the
  I2C-style `PIN_TOUCH_SDA`/`PIN_TOUCH_SCL` naming in the pre-existing (and
  apparently wrong) commented template at the bottom of `config.h` --
  XPT2046 is SPI, not I2C, so that template's naming didn't match its own
  stated driver. `HAS_TOUCH_INPUT=1, HAS_GPIO_BUTTONS=0, HAS_NEOKEY_BUTTONS=0`.
- New capability flag `TOUCH_SHARES_DISPLAY_SPI_BUS` (1 for CYD2USB_DIYMALLS_ILI9341, 0 for
  the other two) -- not used yet at this step (still single-threaded), but
  recorded now so Step 4 knows which boards need bus guarding.
- New `display.h` `#elif defined(BOARD_CYD2USB_DIYMALLS_ILI9341)` LGFX board class:
  `Panel_ILI9341` + `Touch_XPT2046` sharing one `Bus_SPI` (own CS/IRQ pins,
  `bus_shared = true` on both panel and touch config).
- **Build:** new env, plus the two existing envs (unaffected).
- **Test:** CYD2USB_DIYMALLS_ILI9341 touch buttons behave the same as the capacitive CYD
  (press flash + serial log) -- confirms the touch abstraction really is
  chip-agnostic before concurrency is introduced.
- *Status: all three envs (`cerberus-m5-core`,
  `cerberus-esp32-s3-cyd-touch-freenove`, `cerberus-cyd2usb-diymalls-ili9341`) build
  successfully.* **UNVERIFIED, flagged per your instruction:** chip family
  (plain ESP32), flash size (4MB), and every CYD2USB_DIYMALLS_ILI9341 pin number are
  commonly-documented values for the ESP32-2432S028R family, not confirmed
  against your actual board. Also: the touch-calibration data block in
  `application.cpp` (`touchCalData`) is still gated to
  `BOARD_S3_CYD_TOUCH_FREENOVE` only -- it's calibrated for that capacitive
  panel and almost certainly wrong for a resistive XPT2046 panel, so
  CYD2USB_DIYMALLS_ILI9341 currently gets *no* calibration applied (raw touch coords). Needs
  its own calibration pass (the existing commented-out `calibrate(lcd)` call
  in `app_setup()`) before touch positions can be trusted on real hardware.
- **Correction after first hardware test:** reported symptoms were inverted
  colors, streaks, and mirrored text. The streaks traced back to a wrong
  assumption in the original implementation: XPT2046 touch does **not**
  share the panel's SPI pins on this board family -- it has its own
  dedicated bus (`PIN_TOUCH_CLK=25, PIN_TOUCH_MOSI=32, PIN_TOUCH_MISO=39,
  PIN_TOUCH_CS=33, PIN_TOUCH_IRQ=36`, on `SPI2_HOST`, separate from the
  panel's `SPI3_HOST`). `config.h`/`display.h` updated accordingly,
  `TOUCH_SHARES_DISPLAY_SPI_BUS` flipped to 0 for this board, `bus_shared`
  now `false` on both panel and touch. Still unverified against your exact
  unit, but internally consistent with ESP32 GPIO constraints (36/39 are
  input-only, used here only for the two touch-to-ESP32 read signals).
  Inversion and mirroring are separate, still-open panel-tuning items --
  `INVERT_COLORS` and the new `PANEL_OFFSET_ROTATION` (try 0/2/4/6) are
  exposed as named constants in `config.h` for you to iterate on directly
  without needing a code change each time. All three envs still build after
  the fix.

### Step 4 -- Real Local Input Polling Task (Core 1)
Add `xTaskCreatePinnedToCore` task in `application.cpp`, core 1, looping
every `INPUT_POLL_PERIOD_MS` (15ms default, add to config.h), calling
`poll_gpio_buttons()` + `poll_touch_buttons()`. Remove those calls from
`app_loop()` (which now only drains the queue + renders). Because a second
task must not draw, drop the press-flash draw from `poll_touch_buttons()`
(documented loss, revisit later via a PRESSED/RELEASED event type if wanted).
- Where `TOUCH_SHARES_DISPLAY_SPI_BUS` is set (CYD2USB_DIYMALLS_ILI9341 only), add a shared
  FreeRTOS mutex guarding the polling task's `getTouch()` call and the main
  task's draw calls -- this is the bus-contention risk that was only a
  future note before Step 3 existed; it's real now that a resistive/SPI
  touch board is actually in the plan. No-op on the other two boards
  (I2C touch and no touch don't share the bus).
- **Build:** all three envs.
- **Test:** press physical (M5) / touch (CYD, CYD2USB_DIYMALLS_ILI9341) buttons; confirm
  `[INPUT]` dispatch lines still appear promptly with no double-fires or
  drops; no visual press-flash anymore (expected). On CYD2USB_DIYMALLS_ILI9341 specifically,
  bombard touch while a redraw is in flight and confirm no garbled screen /
  panic from SPI bus contention.

### Step 5 -- NeoKey stub producer
New `neokey-buttons.h` gated on `HAS_NEOKEY_BUTTONS` (off for all three
boards so far): `init_neokey_buttons()` / `poll_neokey_buttons()` no-ops,
TODO for the real driver. Rename the already-present `I2C_BUTTON`
placeholder in `InputSource` (input-events.h) to `NEOKEY_BUTTON`. Wire the
no-op calls into the Step 4 polling task.
- **Build:** all three envs (stub compiles out, zero behavior change).
- **Test:** confirm no regression on any board.

### Step 6 -- Central button dispatcher
Add `app-modes.h` with `on_button_event(ButtonID)`. Repoint all 4
`BUTTON_MENU[i].onPress` callbacks in `gui-button.h` to call it instead of
their separate `Serial.println` stubs. For now `on_button_event` just logs
new semantic names: ARM->"PREV", START->"NEXT", GOAL->"ACTION",
RESET->"unused". No state machine yet.
- **Build:** all three envs.
- **Test:** each button on all three boards logs its new semantic name.

### Step 7 -- Relabel touch bar captions
Cosmetic: change `BUTTON_MENU` label strings from ARM/START/GOAL/RESET to
PREV/NEXT/ACTION/-- (enum names unchanged).
- **Build:** both CYD envs (the only boards with a visible bar).
- **Test:** CYD and CYD2USB_DIYMALLS_ILI9341 screens show new captions.

### Step 8 -- De-blockify `font-demo.h`, unify across boards
Split `show_fonts_structured` into `font_demo_enter(LGFX&)`,
`font_demo_render(LGFX&)` (drop its own PREV/NEXT button drawing -- the
shared bar covers it), `font_demo_next()`, `font_demo_prev()`. Remove the
internal blocking loop entirely. Wire `on_button_event` (Step 6) so ARM/START
call `font_demo_prev/next()` and call `font_demo_enter()` once at startup on
**all three** boards (previously CYD-only). `app_loop()` renders on a dirty
flag.
- **Build:** all three envs.
- **Test:** CYD/CYD2USB_DIYMALLS_ILI9341 touch PREV/NEXT pages fonts without blocking
  (confirm the polling task/queue keep working live). M5 physical A/B now
  also page fonts on the M5 screen -- previously impossible since the old
  code never ran there.

### Step 9 -- Supervisor state machine
Extend `app-modes.h`: `enum class AppState { SUPERVISOR, FONT_DEMO }`, a
mode table (`{"Font Demo Viewer", enter_font_demo}` + two dummy entries that
just `Serial.println` and stay put), `supervisor_render()` placeholder
screen, and state-aware dispatch in `on_button_event` (SUPERVISOR:
ARM/START cycle the 3 entries, GOAL runs the selected entry; FONT_DEMO:
ARM/START page fonts, GOAL returns to SUPERVISOR). RESET stays logged as
unused in both states.
- **Build:** all three envs.
- **Test:** all three boards boot to Supervisor placeholder; PREV/NEXT
  cycles 3 entries; ACTION on "Font Demo Viewer" enters font browsing;
  ACTION inside font demo returns to Supervisor; dummy entries print to
  serial and don't change screen.

### Step 10 -- Insertion point: fourth board (CYD + 4-button NeoKey I2C)
Everything above is producer-agnostic (`InputSource` already has
`NEOKEY_BUTTON`, `neokey-buttons.h` already has the right shape from Step 5).
To add this board:
1. New PlatformIO env (e.g. `cerberus-esp32-s3-cyd-neokey`) with
   `HAS_TOUCH_INPUT=1, HAS_GPIO_BUTTONS=0, HAS_NEOKEY_BUTTONS=1`.
2. Drop your existing NeoKey driver code into `neokey-buttons.h`'s
   `init_neokey_buttons()`/`poll_neokey_buttons()`, mapping its 4 keys to
   `BTN_ARM/START/GOAL/RESET` via `input_queue_post(id,
   InputSource::NEOKEY_BUTTON)` -- same contract `gpio-buttons.h` and
   `touch-buttons.h` already use.
3. Add the call into the Step 4 polling task (already present as a no-op
   call site).
No changes needed to `app-modes.h`, `font-demo.h`, or the dispatcher --
they're already input-source-agnostic.
- **Build:** new fourth env.
- **Test:** touch and NeoKey both active simultaneously; confirm both
  producers post correctly-mapped events into the same queue.

## Files touched (steps 1-9)
`config.h`, `common/board-config.h`, `common/board-id.h`, `display.h`,
`platformio.ini`, `input-events.h`, `gpio-buttons.h`, `touch-buttons.h`
(new), `neokey-buttons.h` (new, stub), `gui-button.h`, `app-modes.h` (new),
`font-demo.h`, `application.cpp`.

## Assumptions (flag if wrong)
- Dummy Supervisor modes are fire-and-forget (Serial print, stay put).
- Live press-flash on touch is dropped from Step 4 onward (simplification of
  moving detection into a non-drawing task); can come back later via a
  PRESSED/RELEASED event type.
- `BTN_RESET` is the deliberately-unused 4th button throughout (no ID churn,
  per your call).
- CYD2USB_DIYMALLS_ILI9341's exact pin mapping comes from the "Sunton CYD ESP32-2432S028R"
  block already commented at the bottom of `config.h` -- flag if your
  CYD2USB_DIYMALLS_ILI9341 board differs from that pinout.
