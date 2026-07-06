# Build Fix Progress Notes

Working session goal: go through every project in this repo **one at a time**,
build all of its board environments, fix whatever is broken, and only move to
the next project once the current one builds clean for every target. Do not
jump ahead to another project until the current one is fully green.

If a fix looks like it applies to more than one project (e.g. something in
`base-boards.ini`), it's fine to apply it centrally rather than per-project,
but confirm with the user first if the change is broad (touches the shared
`base-boards.ini` used by all 10 projects).

## Environment quirks discovered (apply throughout)

- **Don't batch envs in one `pio run -e a -e b -e c` call.** Building several
  envs in a single invocation triggers a flaky SCons `.sconsign310.dblite`
  race and can even silently link stale/partial object files (seen as bogus
  `undefined reference to setup()/loop()` errors). Build **one env per `pio
  run` invocation**.
- Use `pio run -e <env> -j 1`. Single-job still occasionally hits the same
  SCons dblite race (`FileNotFoundError: .sconsign310.tmp`) but far less
  often than parallel jobs, and never the stale-object-file failure mode.
  If you see the dblite race, just retry the same command once — it's not a
  real error. Also seen once: `<project>.ino.cpp: No such file or directory`
  (the `.ino`→`.cpp` conversion step racing the compile step) — same deal,
  just retry.
- After confirming a real (non-flaky) failure, `rm -rf .pio/build/<env>`
  before retrying to rule out stale build cache.
- Run builds from inside the project directory (`cd <project> && pio run
  -e <env> -j 1`), not from the repo root.

## Global fixes already applied (affect all projects)

1. **`base-boards.ini:110`** (now shifted a few lines down after edits) —
   `NEOPIXEL_COLOR_ORDER=NE_RGB` was a typo for `NEO_RGB` in `base_s3_zero`.
   Fixed.
2. **`base-boards.ini` `env_common.lib_deps`** — was missing `Networking`.
   Root cause: history shows `env_common.lib_deps` used to contain
   `Networking` (matching `Network/library.properties`'s `name=Networking`
   field) alongside the raw `-I".../Network/src"` build flag. Commit
   `cb0cbb3` ("fix the networking include issue", 2026-07-02) accidentally
   dropped the `Networking` lib_deps line while keeping the now-useless `-I`
   flag. A raw `-I` flag only makes headers visible to the compiler; it does
   NOT make PlatformIO's Library Dependency Finder (LDF) compile the
   library's `.cpp` sources. Without the real dependency, `NetworkInterface`,
   `NetworkEvents` etc. are declared but never compiled in, causing
   **link-time** errors (`undefined reference to NetworkInterface::...`) in
   any project that exercises WiFi beyond trivial use (e.g. `WiFi.scanNetworks()`).
   This is NOT related to the pioarduino platform fork (that was already in
   use before the regression and worked fine).
   Fix applied: restored `lib_deps = Networking` to `env_common`, and
   propagated `${env_common.lib_deps}` into all 8 `base_X` sections'
   `lib_deps` (adding the option to three sections that didn't have one
   before: `base_c3_super_mini`, `base_c3_xiao`, `base_c6_xiao`), mirroring
   the existing `${env_common.build_flags}` convention.
   Verified via `tools/check_ini_composition.py` — no composition issues.
   The `-I".../Network/src"` build flag was left in place (harmless,
   matches the last known-good historical config).

## Per-project status

### Done (all targets build green)

- **ble-serial** — 7/7 envs build (`ble-serial-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). Fixes applied (in `ble-serial/`, not global):
  - Every env in `ble-serial/platformio.ini` now extends `feature_ble` in
    addition to its board base, and merges `lib_deps`/`build_flags` via
    `${base_X.lib_deps} + ${feature_ble.lib_deps}` (three C3/C6 envs that
    don't get `lib_deps` from their base at all only pull
    `${feature_ble.lib_deps}` — that's correct, not an oversight).
  - `application.cpp`: added missing `#include "../common/board-config.h"`
    (was using `SERIAL_BAUD` without including the header that defines it).
  - `application.cpp`: updated `MyServerCallbacks::onConnect/onDisconnect`
    and `MyCallbacks::onWrite` to the NimBLE-Arduino 2.x callback signatures
    (`NimBLEConnInfo&` param added, plus a disconnect `reason` int) since
    `feature_ble` pins `NimBLE-Arduino @ ^2.3.0` (resolves to 2.5.0), and the
    original code targeted the 1.x API. User explicitly chose "update code
    for 2.x API" over "pin lib to 1.4.1" when asked.
  - Tried and rejected: a global `[env]` wrapper section to auto-inject
    `feature_ble` into every env in the file. Doesn't work — PlatformIO only
    falls back to `[env]` defaults when an option has no value at all
    (including via `extends`); since every board env already inherits
    `build_flags`/`lib_deps` from its base, `[env]` defaults get silently
    ignored. Confirmed by direct experiment, not just reasoning.

- **wifi-scanner** — 7/7 envs build (`wifi-scanner-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). Fixed entirely by the central `Networking` lib_deps fix above — no
  per-project changes needed in `wifi-scanner/platformio.ini` itself.

- **blinky-freertos** — 7/7 envs build (`blinky-freertos-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed at all — already fully fixed by the
  two global `base-boards.ini` fixes (`NE_RGB` typo, `Networking` lib_deps).
  Every env hit the flaky SCons dblite race on the first attempt and
  succeeded on retry; no real errors found.

- **wifi-beacon-spammer** — 7/7 envs build (`wifi-beacon-spammer-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed — fully fixed by the two global
  `base-boards.ini` fixes. All 7 succeeded on the first attempt, no flakiness
  hit at all this time.

- **wifi-congestion-meter** — 7/7 envs build (`wifi-congestion-meter-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed — fully fixed by the two global
  `base-boards.ini` fixes. Only `-s3-zero` hit the flaky dblite race (fixed
  on retry); the rest succeeded first try.

- **wifi-udp-blaster** — 7/7 envs build (`wifi-udp-blaster-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed — fully fixed by the two global
  `base-boards.ini` fixes. Only `-s3-zero` hit the flaky dblite race (fixed
  on retry); the rest succeeded first try.

- **event-capture-freertos** — 7/7 envs build (`event-capture-freertos-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed — fully fixed by the two global
  `base-boards.ini` fixes. Only `-s3-zero` hit the flaky dblite race (fixed
  on retry); the rest succeeded first try.

- **hesperus-gate-sensor** — 7/7 envs build (`hesperus-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`). No code/config changes needed — fully fixed by the two global
  `base-boards.ini` fixes. Only `-s3-zero` hit the flaky dblite race (fixed
  on retry); the rest succeeded first try.

- **oled-display** — 10/10 envs build (was 7). Fix applied (in
  `oled-display/platformio.ini`, not global): every env except
  `-s3-cyd-touch-freenove` (which already gets `feature_oled` via its
  base) now also extends `feature_oled`, merging `lib_deps`/
  `build_flags` the same way `ble-serial` did for `feature_ble` — this
  project's `application.cpp` includes `<U8g2lib.h>` unconditionally on
  every board, not just the CYD/touch target.

  On top of that, did a follow-up piece of work: made OLED display
  resolution/controller a proper per-environment build axis instead of a
  hand-edited macro in source (see plan file
  `~/.claude/plans/before-moving-on-i-giggly-elephant.md` for full
  context/rationale). `oled-manager.h`'s `ACTIVE_DISPLAY_INDEX` macro block
  (previously manually commented/uncommented) is now
  `#ifndef ACTIVE_DISPLAY_INDEX / #define ... DISPLAY_SSD1306_128X64 / #endif`,
  overridable via a build flag. Added 4 local (non-`env:`) sections in
  `platformio.ini` — `oled_ssd1306_128x64`, `oled_ssd1306_128x32`,
  `oled_ssd1306_64x32`, `oled_sh1106_128x64` — each just setting
  `-D ACTIVE_DISPLAY_INDEX=N` (values must match the `DISPLAY_*` enum in
  oled-manager.h: 64x32=0, 128x32=1, 128x64=2, sh1106=3). Only the `s3-zero`
  board got all 4 variants as separate envs (`oled-display-esp32-s3-zero-
  ssd1306-128x64`, `-ssd1306-128x32`, `-ssd1306-64x32`, `-sh1106-128x64`);
  the other 6 board envs are untouched and fall back to the header's default
  (`SSD1306_128X64`, i.e. no behavior change for them). I2C pins stay fixed
  at SDA=8/SCL=7 for every variant (matches current physical wiring) — the
  displayTable's per-display pin defaults and the no-arg `initDisplay()`
  overload remain unused/dead code, unchanged, by explicit user choice.

  Verified via `tools/check_ini_composition.py` — no composition issues. All
  10 envs (4 new s3-zero variants + 6 unchanged boards) built successfully
  one at a time; the 6 unchanged boards produced byte-identical flash sizes
  to their pre-change builds (no regression), and the 4 s3-zero variants
  produced distinct flash sizes from each other confirming the build flag
  actually changes which U8g2 class gets compiled in. No dblite flakiness
  hit this round.

- **cerberus-gate-controller** — 2/2 envs build (down from 8). This project
  is a touchscreen/display UI app (bike/gate control panel); 6 of its 8 envs
  had no physical screen and no LovyanGFX dep, and were dropped per user
  decision (`cerberus-esp32-s3-zero`, `-s3-super-mini`, `-c3`, `-c3-xiao`,
  `-c6-xiao`, `-c6`). Kept: `cerberus-esp32-s3-cyd-touch-freenove` (Freenove
  FNK0104B, ILI9341+FT6336U touch, manual LovyanGFX config) and
  `cerberus-m5-core` (M5Stack Core -- **not** Core2; see rename below --
  ILI9341 via LovyanGFX `LGFX_AUTODETECT`, 3 hardware buttons, no touch).

  Full design/rationale in `~/.claude/plans/before-moving-on-i-giggly-elephant.md`.
  Fixes applied:
  - `base-boards.ini`: added `[feature_lovyangfx]` (lib_deps =
    `lovyan03/LovyanGFX @ ^1.1.16`, matching the `feature_ble`/`feature_oled`
    pattern — project-scoped since only cerberus needs it, not merged into
    `base_s3_cyd_touch_freenove`/`base_m5_core` themselves since other
    projects share those bases without needing LovyanGFX).
  - **Board identity correction**: renamed `base_m5_core2`→`base_m5_core`
    and `-D BOARD_M5_CORE2`→`-D BOARD_M5_CORE` throughout (`base-boards.ini`,
    `cerberus-gate-controller/config.h`, `display.h`). The code called this
    board "Core2" but the user confirmed the physical unit has no
    touchscreen and 3 hardware buttons instead — that's the original
    M5Stack Core/Basic/Gray/Fire, not Core2 (which has capacitive touch).
    Confirmed via grep this board is cerberus-exclusive (no other project
    references it), so the rename was safe and total.
  - `cerberus-gate-controller/platformio.ini`: dropped the 6 non-display
    envs; both surviving envs now extend `feature_lovyangfx` and merge
    `lib_deps`/`build_flags` the same way `ble-serial` merged `feature_ble`.
    Also deleted a block of dead TFT_eSPI-style build flags
    (`USER_SETUP_LOADED=1`, `ILI9341_DRIVER=1`, `TFT_WIDTH=240`, etc.) that
    LovyanGFX never reads — the real panel config comes entirely from
    `config.h`'s constexpr pin values.
  - `cerberus-gate-controller/display.h`: replaced the implicit
    `#ifdef BOARD_M5_CORE2 ... #else` (silently defaulting anything
    non-Core2 to the Freenove config — how the 6 dropped envs got a wrong
    display config instead of failing loudly) with an explicit
    `#if defined(BOARD_M5_CORE) ... #elif defined(BOARD_S3_CYD_TOUCH_FREENOVE)
    ... #else #error ... #endif` chain.
  - Verified via `tools/check_ini_composition.py` (clean) and building both
    envs (each needed one retry for this session's known flaky SCons races,
    not real errors) — `LovyanGFX.hpp` resolves and the full app
    (`font-demo.h`, `gui-button.h`, `touch-calibration.h`, previously never
    compiled since the build always failed before reaching them) compiles
    clean on both boards.

  **Explicitly deferred** (touchscreen/input phase, not done yet): the
  touch controller (`Touch_FT5x06`) is a private member instantiated inside
  the Freenove `LGFX` class itself, not an independent swappable axis — that
  needs its own design pass once the user is ready (XPT2046/GT911
  alternates are already documented as commented-out profiles in
  `config.h`). Also deferred: wiring up the M5's 3 physical buttons, and the
  user's broader idea of a unified ~4-button input model. `application.cpp`
  currently calls `lcd.getTouch()` unconditionally on both boards — harmless
  on M5 (no touch hardware, so it just never reports a touch) until that
  phase happens.

## All projects done

Every project in this repo now builds clean on every env it's expected to
support. This file can be treated as a historical record of what was found
and fixed; there's no more "not yet started" work queued.

## Housekeeping done earlier in this session (unrelated to build fixes, FYI)

- Added a `[env:version_metadata]` block (`release_version = ...`) to all 10
  projects' `platformio.ini`, matching `blinky-freertos`'s existing block.
- Triggered a build of the first non-metadata env in each of the 10 projects
  to auto-generate `ARDUINO_GUIDE.md` per project (via the `pre:
  ../tools/auto_compose.py` script in `env_common.extra_scripts`). All 10
  have an `ARDUINO_GUIDE.md` now, though it may need regenerating once more
  builds succeed since the "Required Libraries" section depends on `lib_deps`
  resolution, which has changed since (Networking, feature_ble, etc.).
