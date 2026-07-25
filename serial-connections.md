# Serial Device Connections on ESP32 (Arduino Framework)

A technical guide to how the USB CDC serial connection actually behaves
with the Arduino framework on ESP32 chips -- which class backs `Serial`,
how its connection state is really determined, and the practical
consequences for code (like `blinky-freertos`) that waits on `Serial` or
writes to it before a host is attached. Written against the Arduino-ESP32
core and esptool sources as installed by this template's pinned toolchain
(`espressif32 @ 7.0.1`, see `base-boards.ini`); file/line references are
against those sources unless stated otherwise.

## Contents

1. [USB peripheral background](#usb-peripheral-background)
2. [How `Serial`'s connection state actually works (`HWCDC`)](#how-serials-connection-state-actually-works-hwcdc)
3. [Connection-timing scenarios](#connection-timing-scenarios)
   - [Scenario 1: host resets the target as part of connecting](#scenario-1-host-resets-the-target-as-part-of-connecting)
   - [Scenario 2: target is already running, then a terminal connects](#scenario-2-target-is-already-running-then-a-terminal-connects)
   - [Scenario 3: connected target takes a hard reset (EN pin / power cycle)](#scenario-3-connected-target-takes-a-hard-reset-en-pin--power-cycle)
   - [Scenario 4: closing the terminal doesn't make `Serial` go false quickly](#scenario-4-closing-the-terminal-doesnt-make-serial-go-false-quickly)
4. [Other practical gotchas](#other-practical-gotchas)
   - [Mismatched USB mode on dual-peripheral boards](#mismatched-usb-mode-on-dual-peripheral-boards)
   - [The ROM bootloader and the sketch are different CDC devices](#the-rom-bootloader-and-the-sketch-are-different-cdc-devices)
   - [Only one host process can hold the port open](#only-one-host-process-can-hold-the-port-open)
   - [Power-only USB cables](#power-only-usb-cables)
5. [Comparison: external USB-to-serial bridge (CH340, CP2102, FTDI)](#comparison-external-usb-to-serial-bridge-ch340-cp2102-ftdi)
6. [Open questions (not yet verified)](#open-questions-not-yet-verified)

## USB peripheral background

Which C++ class actually backs `Serial` depends on `ARDUINO_USB_MODE`,
and which values of that flag are even valid depends on which USB
peripheral(s) the target chip has. Get this settled first -- everything
else in this guide follows from it.

`ARDUINO_USB_MODE` is a plain boolean throughout the Arduino core --
every use is `#if ARDUINO_USB_MODE` or `#if !ARDUINO_USB_MODE`. There is
no third mode. Checked `soc_caps.h` across the family (C5/C6 rows
verified differently -- see note below):

| Chip            | OTG peripheral | Serial-JTAG peripheral |
|-----------------|----------------|-------------------------|
| ESP32 (classic) | no             | no                      |
| ESP32-S2        | yes            | **no**                  |
| ESP32-S3        | yes            | yes                     |
| ESP32-C3        | no             | yes                     |
| ESP32-C6        | no             | yes                     |
| ESP32-C5        | no             | yes                     |

`HWCDC.cpp` (Mode 1) is only compiled at all
`#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3` (line 15) --
it doesn't exist as an option on S2, which never got the Serial-JTAG
peripheral. Classic ESP32 has neither peripheral: no native USB CDC of
any kind, and `ARDUINO_USB_MODE` is irrelevant there -- it's always an
external UART bridge chip (see
[Comparison](#comparison-external-usb-to-serial-bridge-ch340-cp2102-ftdi)).

> **C5/C6 verification note.** The framework version this template is
> pinned to doesn't ship an SDK for either chip locally, so `soc_caps.h`
> couldn't be checked directly for them. Confirmed instead via esptool's
> own target definitions (installed alongside this toolchain):
> `esp32c6.py` declares `class ESP32C6ROM(ESP32C3ROM)`, and `esp32c5.py`
> declares `class ESP32C5ROM(ESP32C6ROM)` -- both inherit
> `uses_usb_jtag_serial()` and the Serial-JTAG GPIO handling straight
> from C3's class, and neither file defines `uses_usb_otg()` or
> `UARTDEV_BUF_NO_USB_OTG` (unlike `esp32s3.py`/`esp32s2.py`, which both
> define those). Both also carry explicit GPIO warnings for the
> Serial-JTAG peripheral (C6: GPIO 12/13; C5: GPIO 13/14). Consistent
> picture: C6 and C5 both follow the C3 model, so the same "only Mode 1
> exists" conclusion applies to them, though this template doesn't
> currently build for either (see the C6-disabled note in
> `base-boards.ini`).

**Mode 1 (`ARDUINO_USB_MODE=1`)** -- what every board in this template
uses. `Serial` is a global `HWCDC` instance (`HWCDC.cpp:605-611`), backed
by the chip's built-in, fixed-function USB-Serial-JTAG peripheral: just a
serial channel plus JTAG debug access, nothing else.

**Mode 0 (`ARDUINO_USB_MODE=0`, or left unset)** -- only valid on chips
with a full USB-OTG peripheral (S2 or S3, not C3/C6/C5). `Serial` is a
global `USBCDC` instance instead (`USBCDC.cpp:449-451`), backed by the
full TinyUSB stack. `main.cpp:57-68` shows what that unlocks that Mode 1
cannot do at all: `MSC_Update.begin()` (expose flash as a USB mass
storage device), `USB.enableDFU()`, and generic `USB.begin()` for
composite/custom USB descriptors (HID and similar). Its connection state
comes from genuine DTR/RTS control-line signalling (`_onLineState()`),
which is also what implements the software reset-into-bootloader dance
described in [Scenario 1](#scenario-1-host-resets-the-target-as-part-of-connecting).

So the choice isn't "Mode 1 is always the safe default" -- it's
chip-dependent:
- **C3 / C6 / C5**: only Mode 1 exists (no OTG hardware for Mode 0).
- **S2**: only Mode 0 exists (no Serial-JTAG hardware for Mode 1).
- **S3** (and presumably later dual-peripheral chips): a genuine choice.
  Mode 1 is the simpler, smaller-surface-area option if MSC/DFU/HID
  aren't needed; Mode 0 is what unlocks those, at the cost of the more
  complex composite-device stack.

This template sets `ARDUINO_USB_MODE=1` for every board, including its
S3 profiles (`base-boards.ini`), so every board here follows the `HWCDC`
model for the rest of this guide -- `USBCDC` is discussed only for
contrast, and is never actually exercised by anything this template
builds.

## How `Serial`'s connection state actually works (`HWCDC`)

`HWCDC`'s `Serial` boolean (`isCDC_Connected()`) does **not** track
DTR/RTS at all -- there is no such logic anywhere in `HWCDC.cpp`. That's
a real difference from `USBCDC` (Mode 0), which does track DTR/RTS; if
you're used to reasoning about CDC-ACM control lines from other Arduino
boards, that model doesn't apply here.

Instead, `HWCDC`'s internal `connected` flag is inferred purely from
live USB bulk-transfer activity:
- Set **`true`** by the ISR (`hw_cdc_isr_handler`) once the host actually
  picks up data the device transmitted, or sends data to the device.
- Set **`false`** only on a genuine USB bus reset (`BUS_RESET`
  interrupt), or when a separate physical-presence check (`isPlugged()`,
  driven by whether the host is still sending USB Start-of-Frame packets
  roughly every 1ms) goes false.

Neither of those is the same thing as `Serial.begin()` completing -- the
peripheral can be fully configured on the chip side while `Serial` still
reads `false`, because none of the above conditions have been met yet.
This is the mechanism behind every scenario below.

**Writing while disconnected.** Writes made while `Serial` is `false`
are not queued reliably and not resent later: `HWCDC::write()`'s
disconnected path (`flushTXBuffer()`) does a non-blocking insert into a
small TX ring buffer, evicting the oldest queued bytes once it fills.
Nothing is transmitted, and nothing times out -- the data just silently
evaporates. (`Serial.setTxTimeoutMs()` plays no part in this path
either; see [Scenario 2](#scenario-2-target-is-already-running-then-a-terminal-connects)
for what it actually governs.)

**Checking connection state in code: prefer `isConnected()` over the
bare bool.** `if (Serial)` works because `HWCDC` defines
`operator bool() const`, but that operator is a one-line pass-through to
a single underlying method: `HWCDC.cpp:289-291` shows it just calls the
same private `isCDC_Connected()` that everything else in this section
describes. `HWCDC.h` also exposes that method publicly under a clearer
name -- `static bool isConnected(void)` (line 72) -- which is the exact
same call, not a different check. `if (Serial.isConnected())` and
`if (Serial)` are therefore identical at runtime; the only difference is
that the former says what's actually being tested.

That distinction is worth making explicit because the bare form invites
a natural misreading: a bool conversion operator on an object
conventionally answers "has this been constructed/initialized," not
"what is this object's current, fast-changing runtime state." `Serial`
overloading `operator bool()` to mean "is a host currently connected" is
a deliberate, long-standing Arduino convention -- native-USB boards going
back to the 32u4-based Leonardo/Micro did the same -- but it still reads
as a surprise if you're not expecting it. `blinky-freertos/application.cpp`
uses `Serial.isConnected()` throughout for this reason.

## Connection-timing scenarios

The four scenarios below all answer the same question -- "how quickly,
and why, does `Serial` reflect reality?" -- for the four ways a host
connection typically starts or ends.

### Scenario 1: host resets the target as part of connecting

Upload tools do reset these boards via a DTR/RTS toggle sequence --
confirmed directly in esptool's own source (`esptool/reset.py`), which
has a dedicated `USBJTAGSerialReset` strategy specifically for boards
using the USB-Serial-JTAG peripheral (as distinct from `ClassicReset`,
used for external UART bridge chips like CH340 whose EN/IO0 pins are
wired to DTR/RTS through a hardware auto-reset circuit). Its `reset()`
toggles DTR and RTS through a short timed sequence to force the chip
into, then out of, the bootloader.

This is a different, lower-level mechanism than `USBCDC`'s
`_onLineState()` DTR/RTS handling -- that code lives in the Arduino
core's `USBCDC.cpp` and only applies when `ARDUINO_USB_MODE=0`, which
none of this template's boards use. Whatever actually interprets
esptool's DTR/RTS sequence for a USB-Serial-JTAG-peripheral board sits
below the Arduino sketch layer (likely ROM or IDF driver code) and
hasn't been traced in this codebase -- flagged as unverified at that
level of detail.

What is confirmed is the practical consequence: the same host process
driving the reset is the one already sitting on the port with the reset
sequence in flight, so by the time freshly-flashed firmware boots and
reaches code that waits on `Serial`, the host is already present.
`Serial` should evaluate `true` almost immediately.

Caveat: "almost immediately" is not zero. After the reset, the USB
peripheral drops off the bus and has to re-enumerate -- the host OS sees
the device disappear and reappear, which takes on the order of tens to a
few hundred milliseconds, plus however long the host tool takes to notice
and reopen the port. That gap is real but is an order of magnitude
shorter than a multi-second startup timeout, so it doesn't matter in
practice.

### Scenario 2: target is already running, then a terminal connects

Here the chip has already booted and been running disconnected for some
time (or indefinitely). The USB port is already enumerated at the OS
level; what's still pending is a terminal application actually opening
the port and beginning to service it.

For `HWCDC` there's no DTR/RTS event to react to, so "opening the port"
isn't quite what flips `Serial` to `true`. While disconnected,
`isCDC_Connected()` flushes the TX FIFO on every call -- i.e. every time
application code checks `Serial` -- to try to provoke a
`SERIAL_IN_EMPTY` interrupt, which only fires once the host's driver
actually pulls data from the bulk-IN endpoint. In practice that happens
as soon as a terminal opens the port, since the host OS's CDC-ACM driver
starts servicing the endpoint on open. So the observable behaviour
matches what you'd expect -- `Serial` goes `true` once a terminal
connects -- it's just proven by successful data flow rather than by a
control-line signal.

Anything written to `Serial` before that point is not "sent, then timed
out, then dropped" -- it never attempts transmission at all (see
[the write-while-disconnected note above](#how-serials-connection-state-actually-works-hwcdc)).
The `tx_timeout_ms` mechanism (`Serial.setTxTimeoutMs()`) plays no part
in that disconnected path; it only matters for writes made while
*already connected*, where the buffer is full and the host isn't
draining it fast enough.

### Scenario 3: connected target takes a hard reset (EN pin / power cycle)

A terminal is attached and working, then the ESP32 is hard-reset via its
EN pin or a power cycle. Unlike Scenario 1, this reset has no software
participation at all -- the USB-Serial-JTAG peripheral itself drops off
the bus and the host is forced to re-enumerate from scratch. Nothing told
the host this was coming, so there is no reason to expect it to be ready
quickly the way Scenario 1's uploader is.

Because of that, code waiting on `Serial` after this kind of reset should
be treated like Scenario 2's indeterminate wait, not Scenario 1's
near-instant one -- and the wait is arguably longer, since it now
includes OS-level USB re-enumeration on top of whatever the terminal
emulator needs to do to notice and reopen the port.

Whether the host reuses the same device name (e.g. `/dev/ttyACM0`, a
Windows COM port) for the re-enumerated device is outside anything the
ESP32 firmware controls. It depends on host OS device-naming policy,
whether the device exposes an identifier the OS keys off, and whether the
old device node had finished tearing down before the new one appeared --
genuinely "may or may not," and not something the firmware side can
influence.

A terminal emulator's ability to recover is entirely tool-specific:
- If the device re-enumerates under the same name, a client that
  retries against that path can resume once it reappears.
- If the name changes, path-based reconnect logic fails outright and
  usually needs a human to pick the new port.

### Scenario 4: closing the terminal doesn't make `Serial` go false quickly

Observed directly in `blinky-freertos` with the status LED wired to poll
`Serial` on every `app_loop()` iteration: starting with no terminal
connected shows red, connecting a terminal flips it to green -- but
closing the terminal again leaves it green for several seconds before it
finally goes red.

This follows directly from how `HWCDC` derives `connected` (see
[above](#how-serials-connection-state-actually-works-hwcdc)): there is
no signal at all for "the application on the host closed its file
handle." Closing a terminal doesn't stop the host's USB controller
sending Start-of-Frame keep-alives -- the cable's still plugged in, the
bus is still live -- so `isPlugged()` stays `true`, and closing an
application doesn't necessarily trigger a `BUS_RESET` either. The
firmware can only notice the loss once one of those two real events
actually happens.

Confirmed: the multi-second lag is real and reproducible. Not verified:
the exact host-side event chain that eventually trips it -- that's down
to the host OS/driver eventually tearing down or resetting the USB
interface after the last handle closes, which is outside this codebase
and hasn't been traced.

## Other practical gotchas

These aren't about connection timing -- they're separate, unrelated ways
serial-over-USB on ESP32 can bite you.

### Mismatched USB mode on dual-peripheral boards

Checked in `soc_caps.h`: ESP32-C3 only has `SOC_USB_SERIAL_JTAG_SUPPORTED`
-- no native USB-OTG peripheral exists on that chip, so there's no
ambiguity about which one `Serial` uses. ESP32-S3 has both
`SOC_USB_OTG_SUPPORTED` and `SOC_USB_SERIAL_JTAG_SUPPORTED`, and
`esp32-hal-tinyusb.c` shows the two peripherals are muxed onto the same
physical D+/D- pins through a PHY-select register
(`RTC_CNTL_SW_USB_PHY_SEL` and friends) -- only one is ever electrically
connected at a time, chosen by the `ARDUINO_USB_MODE` build flag.

This template's S3 board profiles (`base_s3_zero`, `base_s3_super_mini`,
`base_s3_cyd_touch_freenove`) all build with `ARDUINO_USB_MODE=1`
(USB-Serial-JTAG / `HWCDC`). If that flag ever ended up mismatched with
whichever peripheral a given board's USB connector is actually wired to,
the result is total silence -- no enumeration at all. That is a
build-configuration fault, not a timing race like the scenarios above,
and needs a different fix (check the build flag, not the wait logic).

### The ROM bootloader and the sketch are different CDC devices

During upload, the chip's ROM bootloader runs its own USB-CDC
implementation to talk to esptool -- this is not the sketch's `HWCDC`
`Serial` object, which doesn't exist yet at that point. A terminal that
appears to hold "the same port" across a flash cycle is actually talking
to two different pieces of software in sequence: the ROM downloader
during flashing, then the application's own CDC stack once the new
firmware boots and runs `Serial.begin()`. Worth keeping in mind when
reasoning about why behaviour around a flash cycle can look different
from a plain runtime reset
([Scenario 1](#scenario-1-host-resets-the-target-as-part-of-connecting)).

### Only one host process can hold the port open

The CDC device node can only be opened by one process at a time. A
terminal emulator left open on the port will make esptool's upload fail
to even open it, independently of any DTR/RTS handshake -- this is
ordinary OS file-handle exclusivity, not a CDC-specific behaviour, but
it's a common practical cause of "upload failed" reports.

### Power-only USB cables

A charge-only USB cable (no data lines) means the device never
enumerates at all. `Serial` stays `false` indefinitely, not just for a
while. This is the one case where an unbounded wait on `Serial` truly
never resolves -- code that waits forever for a connection (rather than
on a bounded timeout, as in the loop removed from `blinky-freertos`)
will hang permanently under this condition.

## Comparison: external USB-to-serial bridge (CH340, CP2102, FTDI)

Everything above is specific to native USB CDC (`HWCDC` here, `USBCDC`
on `ARDUINO_USB_MODE=0` builds not used in this template), where
`Serial`'s truthiness is backed by some form of live connection state fed
from the host side. Boards using an external USB-to-serial bridge chip --
CH340, CP2102, FTDI, wired to the ESP32's UART0 -- work on a
fundamentally different model.

`Serial::operator bool()` for the UART case (`HardwareSerial.cpp`) is
just `return uartIsDriverInstalled(_uart);` -- it reflects whether
`Serial.begin()` has configured the UART driver, full stop. It has no
knowledge of the bridge chip's USB enumeration state, no DTR/RTS concept
reaching the firmware, and no notion of a terminal being open on the
host. Once `begin()` runs, `Serial` reads `true` and stays `true` for the
life of the sketch, whether or not a bridge is attached, its USB side has
enumerated, or anyone is reading.

Writes behave differently at a structural level too, not just by
happening to report success. `HardwareSerial::write()` clocks bytes into
the UART's TX FIFO, which drains at a fixed rate set by the configured
baud rate, driven purely by the chip's own hardware timing -- unlike a
CDC endpoint, which only drains when a host's USB stack actively pulls
from it. There is no backpressure path from "nobody is reading the other
end" back to the UART peripheral, so there is nothing for `write()` to
block on or fail against. Bytes leave the TX pin regardless of whether a
bridge chip, a host OS, or a terminal application is present on the other
side.

So the target genuinely cannot tell whether anything is listening. The
one caveat is around bootloader mode: it isn't that `Serial` evaluates
`false` while the chip is in the ROM downloader -- it's that the sketch
(and its `Serial` object) isn't running at all during that time. The
classic auto-reset circuit some of these bridge boards use (DTR/RTS
through an RC network into EN and IO0) hands UART0 to the ROM bootloader
on its own terms; there's no firmware-level state to query because no
firmware is executing until the bootloader hands control back.

## Open questions (not yet verified)

- **RX side.** Everything above concerns the ESP32-to-host (TX)
  direction. There's a symmetric question about host-to-device (RX)
  data: what happens to bytes the host sends before the sketch calls
  `Serial.available()`/`read()`, including during the disconnected
  windows described above? Not yet traced through the RX ring buffer
  code -- shouldn't be assumed identical to the TX case.

- **Accidental resets from terminal line-state handling.** esptool's own
  `USBJTAGSerialReset` strategy
  ([Scenario 1](#scenario-1-host-resets-the-target-as-part-of-connecting))
  proves DTR/RTS toggling is a real, working reset trigger for boards
  using the USB-Serial-JTAG peripheral. So a terminal application whose
  own default open/close behaviour happens to walk through a similar
  DTR/RTS pattern could plausibly cause an unwanted reset the same way.
  Whatever component actually interprets that sequence (below the
  Arduino/`HWCDC` layer) hasn't been traced here, so the precise trigger
  conditions remain unverified -- flagged as plausible, not confirmed.
