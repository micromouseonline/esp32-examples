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
  real error.
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

### Not yet started (systematic per-env build pass)

These have NOT had the full "build every env" treatment yet. An earlier,
less rigorous pass (before this systematic project-by-project process
started) found some of these errors, listed below — they still need
verifying/fixing properly, env by env, and may have more once you dig in
(e.g. wifi-scanner also *looked* fine on a single-env smoke test before the
Networking root cause was found — don't assume a single successful env means
the whole project is clean).

- **blinky-freertos** — envs: `blinky-freertos-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Not yet build-tested since the `Networking` fix landed.

- **cerberus-gate-controller** — envs: `cerberus-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`, `-m5-core` (8 envs — has an extra M5 Core2 target the others don't).
  Earlier smoke test found: `application.cpp` / `display.h:23` —
  `fatal error: LovyanGFX.hpp: No such file or directory`. Not yet root-caused
  or fixed (needs the `lib_deps` for LovyanGFX added somewhere, likely a new
  `feature_lovyangfx` block in `base-boards.ini` mirroring `feature_display`,
  since `feature_display` currently only pulls in U8g2 not LovyanGFX — check
  which display library this project actually needs on which envs).

- **event-capture-freertos** — envs: `event-capture-freertos-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Only smoke-tested on `-s3-zero` before the NE_RGB/Networking fixes;
  needs full re-verification.

- **hesperus-gate-sensor** — envs: `hesperus-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Same as above — only smoke-tested on `-s3-zero`, needs full pass.

- **oled-display** — envs: `oled-display-esp32-s3-zero`, `-s3-super-mini`,
  `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`, `-c6`. Earlier
  smoke test found: `application.cpp:2` — `fatal error: U8g2lib.h: No such
  file or directory` on `-s3-zero`, which is odd since `feature_display`
  (U8g2) should already be wired into `base_s3_zero`... actually it's NOT:
  only `base_s3_cyd_touch_freenove` extends `feature_display` in
  `base-boards.ini`. This project needs U8g2 on every target, not just the
  CYD/touch board, so `oled-display/platformio.ini` likely needs the same
  per-env `feature_display` merge treatment that `ble-serial` got for
  `feature_ble` (see pattern in that file).

- **wifi-beacon-spammer** — envs: `wifi-beacon-spammer-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Only smoke-tested on `-s3-zero`; needs full pass (should now
  benefit from the Networking fix automatically, but verify).

- **wifi-congestion-meter** — envs: `wifi-congestion-meter-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Only smoke-tested on `-s3-zero`; needs full pass.

- **wifi-udp-blaster** — envs: `wifi-udp-blaster-esp32-s3-zero`,
  `-s3-super-mini`, `-s3-cyd-touch-freenove`, `-c3`, `-c3-xiao`, `-c6-xiao`,
  `-c6`. Only smoke-tested on `-s3-zero`; needs full pass.

## Suggested order

Alphabetical/arbitrary is fine, but doing the WiFi-only projects
(`wifi-beacon-spammer`, `wifi-congestion-meter`, `wifi-udp-blaster`,
`blinky-freertos`, `event-capture-freertos`, `hesperus-gate-sensor`) before
the two display-dependent ones (`oled-display`, `cerberus-gate-controller`)
is probably fastest, since the display ones need a genuinely new
`feature_lovyangfx`-style fix worked out first.

## Housekeeping done earlier in this session (unrelated to build fixes, FYI)

- Added a `[env:version_metadata]` block (`release_version = ...`) to all 10
  projects' `platformio.ini`, matching `blinky-freertos`'s existing block.
- Triggered a build of the first non-metadata env in each of the 10 projects
  to auto-generate `ARDUINO_GUIDE.md` per project (via the `pre:
  ../tools/auto_compose.py` script in `env_common.extra_scripts`). All 10
  have an `ARDUINO_GUIDE.md` now, though it may need regenerating once more
  builds succeed since the "Required Libraries" section depends on `lib_deps`
  resolution, which has changed since (Networking, feature_ble, etc.).
