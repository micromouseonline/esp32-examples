# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Raw 802.11 beacon flood tool for WiFi resilience stress testing. Built to stress
the user's KRONOS race-timer system (`~/dev/esp32/kronos` -- ATLAS router AP,
CERBERUS gate controller running a WebSocket server, HESPERUS timing-gate
WebSocket clients). KRONOS's own `docs/TEST-TOOLING.md` lists "congested-airtime
stress testing" as wanted-but-unbuilt tooling; this targets two weak points its
docs flag: HESPERUS's `networkQueue` is only 10 deep (drops under sustained
load), and a since-fixed bug where an AP radio blip reset the AP's TSF epoch and
silently gated gate triggers for up to 5 minutes.

**Scope: beacon flood only.** No deauth, association flood, or second-AP channel
interference -- those are documented ideas below, not implemented.

**Legal/regulatory caveat:** this transmits real 2.4GHz RF at high packet rate.
Only run it against your own equipment, on spectrum/premises you own or are
explicitly authorized to test on. Never against third-party networks or in
shared/public airspace.

## Build and flash

PlatformIO project targeting 5 board environments defined in [platformio.ini](platformio.ini):

- `wifi-beacon-spammer-esp32-s3-zero`
- `wifi-beacon-spammer-esp32-s3-super-mini`
- `wifi-beacon-spammer-esp32-s3-cyd-touch-freenove`
- `wifi-beacon-spammer-esp32-c3` -- ESP32-C3 DevKitM-1 (configured as C3 Super Mini)
- `wifi-beacon-spammer-esp32-c3-xiao`

ESP32-C6 envs are commented out repo-wide -- see `../base-boards.ini`'s comment
for why (dropped the pioarduino fork for cross-machine package-cache reliability).

```bash
# Build
pio run -e wifi-beacon-spammer-esp32-c3

# Build and upload
pio run -e wifi-beacon-spammer-esp32-c3 --target upload

# Serial monitor
pio device monitor -e wifi-beacon-spammer-esp32-c3

# Regenerate VSCode IntelliSense config after adding/removing libraries
pio init --ide vscode
```

Upload/monitor port auto-detected via USB CDC.

## Source layout

Everything lives at the project root, not under `src/`:

- `wifi-beacon-spammer.ino` -- Arduino entry point; calls `app_setup()` / `app_loop()`
- `application.cpp` / `application.h` -- beacon flood logic (see below)
- `beacon-frame.h` -- raw 802.11 beacon frame byte-layout builder, MAC randomizer
- `../common/board-config.h` -- multi-board LED pin mapping (`STATUS_LED`)
- `../common/secrets.h` -- gitignored; its `ssid` constant (currently `"juno"`,
  KRONOS's real AP) is reused as "the SSID to scan for and lock the target
  channel to". `password` is unused by this project.

## How it works

Reference target: **ESP32-S3 Zero** (`wifi-beacon-spammer-esp32-s3-zero`), which
has an onboard NeoPixel status LED (`STATUS_LED=21`, `HAS_NEOPIXEL=1`) and a
boot button wired to GPIO0 that pulls low when pressed -- both driven through
the shared `common/status-led.h` / `common/button.h` helpers, so this degrades
gracefully (but without a GPIO0 button wired up) on the other board envs.

**TX power is capped at setup** (`BEACON_TX_POWER`, currently `WIFI_POWER_8_5dBm`
via `WiFi.setTxPower()`) -- confirmed by field testing that sustained near-100%-
duty-cycle transmission at full power (~19.5dBm) browns out an S3 Zero even on a
powered USB hub within about a second of starting the flood (`esp_reset_reason()`
returned `ESP_RST_BROWNOUT`). Since this tool is meant for close-range use next
to the equipment under test, not range, the cap is a deliberate tradeoff -- raise
`BEACON_TX_POWER` only if running from a supply that can actually sustain it.

Boot does not wait for a host serial monitor to attach -- `Serial.begin()` is
fired-and-forgotten so the device runs standalone in the field, not just
tethered to a PC. All `Serial` output is still there if something is listening.

`BEACON_CHANNEL_OFFSET` (default 0) shifts the transmit channel away from the
auto-detected target channel at boot -- 0 tests co-channel airtime contention
(the default; target's radio shares CSMA/CA with the flood, so it "fairly"
loses most of its airtime), a nonzero offset tests adjacent/nearby-channel
interference instead (target's radio doesn't recognize an off-channel
transmitter as sharing the medium, so it doesn't defer at all -- the effect is
raw noise/corruption rather than orderly turn-taking). 2.4GHz channels are only
5MHz apart but ~20-22MHz wide, so anything within roughly +/-4 channels still
overlaps substantially; only +/-5 or more (e.g. 1/6/11 spacing) is genuinely
non-overlapping. This only affects the boot-time auto-lock -- the runtime
Serial channel override always sets an absolute channel, ignoring the offset.

1. On boot (status LED **blue** = booting/searching), scans for `secrets.h`'s
   `ssid` (reusing the same scan/print-table logic the project used to ship as
   a plain WiFi scanner) and locks the radio (`esp_wifi_set_channel`) to
   whatever channel it's found on.
2. If not found, blocks with a Serial prompt asking for a channel number (1-13)
   instead of hanging silently -- note this still requires a serial connection
   to unblock; with no target SSID match and nobody attached, it will wait
   indefinitely with the LED stuck blue.
3. Pre-builds a pool of `BEACON_POOL_SIZE` (32) fake APs at setup: each gets a
   random locally-administered MAC and an SSID of the form
   `KRONOS-STRESSTEST-XXXX` (last two MAC bytes as hex) -- self-labeled as test
   traffic rather than disguised, so it's obviously attributable if anyone else
   scans nearby. Pool identities are stable for the run.
4. Once the channel is locked and the pool is built, status LED goes **green**
   = passive/ready. Pressing the GPIO0 button toggles between passive (green)
   and spamming (LED red) states; press again to stop. A dedicated FreeRTOS
   task (`vBeaconFloodTask`, pinned to core 1, matching `wifi-udp-blaster`'s
   pattern) idles while passive and, once spamming, transmits the whole pool
   back-to-back via
   `esp_wifi_80211_tx()` in a tight loop, yielding once per full pool pass to
   avoid starving the watchdog.
5. While spamming, `app_loop()` prints `beacons/s | MB/s | channel N | pool
   size` stats once a second. At any time (passive or spamming), it polls
   Serial for a typed channel number (1-13) + Enter to retarget at runtime
   without reflashing -- useful when testing at a venue using a different
   router/channel.

No promiscuous mode is used or needed -- `esp_wifi_80211_tx` is a standalone TX
path; enabling promiscuous RX would be pure overhead for a transmit-only tool.

## Roadmap / not built

Documented for later, not implemented in this pass:

- **Deauth-frame injection** -- would directly test CERBERUS/HESPERUS's
  reconnect/backoff paths (a known area of interest per KRONOS's own issue
  docs), but is more aggressive than beacon flooding and was deliberately left
  out of this pass.
- **Airtime saturation via legit traffic** -- point the existing
  `../wifi-udp-blaster` project at CERBERUS's real IP/port to add sustained UDP
  load on top of the beacon flood.
- **Association-request flood / client churn** -- repeatedly join+idle as STA
  clients against the real AP to test its client-table capacity (consumer APs
  often cap around 20-32 clients).
- **Second-AP channel interference** -- flash `../wifi-soft-ap`'s pattern onto a
  spare board as an AP on an overlapping channel.

## Local library

- `lib/esp32-info` -- utility to print chip/flash/heap info to Serial (`getInfo()`, `printFactoryMac()`, `printFlashChipMode()`); not currently called in `application.cpp`

## Do not read or modify

- `.pio/` -- generated build artifacts and downloaded library dependencies
