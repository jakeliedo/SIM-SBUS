# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware (PlatformIO / Arduino framework) for an ESP32-S3 that turns an RC receiver signal (SBUS, PPM, or 6-channel PWM — e.g. from a Radiolink R6DS) into a **native USB HID joystick** recognized by a PC/Steam as a flight-sim controller. No companion PC-side app exists in this repo; the ESP32-S3 enumerates directly as a USB HID device over its native USB-OTG peripheral.

## Build / flash / monitor

Standard PlatformIO CLI (no test suite, lint config, or CI in this repo):

```
pio run                 # build
pio run -t upload       # flash (upload_protocol = esptool, 921600 baud)
pio device monitor      # serial monitor (115200 baud) — UART port only, not the USB-OTG port
pio run -t clean
```

Only one env is defined: `[env:sim-sbus]` (see [platformio.ini](platformio.ini)), so `pio run` needs no `-e` flag.

## Hardware/build constraints that shape the code

- **Board must be `esp32s3usbotg`**, not the more common `esp32-s3-devkitc-1`. Only `esp32s3usbotg` has `build.usb_mode=0` (native TinyUSB) by default in `boards.txt`; `esp32-s3-devkitc-1` defaults to `usb_mode=1` (HWCDC) and will *not* produce a working HID device. This was a real bug fixed in git history (`6fe5d12`) — don't "fix" the board back to devkitc-1.
- Target hardware is a "Super Mini ESP32-S3" module: 4MB flash, QIO mode (not DIO), no external PSRAM. `board_build.*` overrides in [platformio.ini](platformio.ini) exist specifically to match this module, not the generic devkit.
- USB D+/D- are fixed to GPIO20/19 by the SoC's native USB-OTG peripheral and are not configurable in code.
- The UART port (CH340/CP2102) is only used for flashing/serial monitor — it is a separate physical connection from the USB-OTG port that carries the HID traffic.

## Architecture

`src/main.cpp` runs a single mode state machine over three interchangeable RC-decoder backends, all normalized to the same interface (µs pulse widths in, HID axis values out):

```
sbus.cpp / ppm.cpp / pwm_input.cpp  →  main.cpp (mode select + µs→axis mapping)  →  hid_joystick16.cpp  →  USB host
                                              ↑
                                         buzzer.cpp (audible feedback)
```

- **Three RC decoders share one contract**, defined per-backend but symmetric: `xxxBegin()`, `xxxUpdate()` (non-blocking tick), `xxxGetChannel(i)` (returns µs pulse width), `xxxIsConnected()` (signal-present check, `RC_TIMEOUT_MS` = 500ms). All three are initialized in `setup()` regardless of which mode is active — only the active one is read each loop iteration (see `getChannels()` in [main.cpp](src/main.cpp)). Switching modes is just switching which decoder's channels are read; no re-init happens.
  - [sbus.cpp](src/sbus.cpp): UART1, 100 kbaud/8E2/inverted, parses the 25-byte SBUS frame (16 × 11-bit channels) in the main-loop-driven `sbusUpdate()` — not an ISR.
  - [ppm.cpp](src/ppm.cpp): single GPIO ISR on rising edges, distinguishes channel pulses from the inter-frame sync gap by pulse width (`PPM_SYNC_US` threshold). All shared state is `volatile`.
  - [pwm_input.cpp](src/pwm_input.cpp): one ISR per channel (6 total — Arduino's `attachInterrupt` needs a plain function pointer per pin, hence the `isr0..isr5` wrapper table), each timing its own rising→falling edge.
- **Mode switching**: a single button (boot button, GPIO0) short-press cycles `MODE_SBUS → MODE_PPM → MODE_PWM → ...` (`handleButton()` in [main.cpp](src/main.cpp)); debounce/long-press thresholds are in [config.h](src/config.h). There's no persistence — it always boots into `MODE_SBUS`.
- **µs → HID axis mapping**: `rcToAxis()` in [main.cpp](src/main.cpp) linearly maps 1000–2000µs (`RC_MIN_US`/`RC_MID_US`/`RC_MAX_US` in [config.h](src/config.h)) to signed 16-bit `-32767..32767`.
- **HID layer** ([hid_joystick16.cpp](src/hid_joystick16.cpp)/[.h](src/hid_joystick16.h)): a custom `USBHIDDevice` subclass, *not* the Arduino-ESP32 built-in `USBHIDGamepad` (that class only supports `int8_t` axes / 256 steps). It hand-builds a raw TinyUSB HID report descriptor for 6 × 16-bit axes + 16 buttons + 1 hat, packed into a 15-byte report struct. If you touch the report descriptor, the `hid_joystick16_report_t` struct layout, byte offsets, and bit widths must stay in lockstep — a mismatch produces a HID device Windows/Steam can enumerate but reads garbage from.
- **Buzzer** ([buzzer.cpp](src/buzzer.cpp)): non-blocking beep-sequence player (`buzzerUpdate()` polled from the main loop, driven by LEDC PWM tone). Used for USB-connected, mode-changed, and signal-acquired/lost feedback — has its own small ring-buffer queue of `{freq, ms}` steps; don't call `ledcWrite`/`ledcWriteTone` on the buzzer pin directly elsewhere or it'll conflict with the queue.
- **Pin/tuning constants** all live in [config.h](src/config.h) (pin assignments, RC pulse-width range, HID axis range, debounce timing) — check there before hardcoding a GPIO number or threshold elsewhere.

## Gotchas specific to this codebase

- ISR-shared variables in `ppm.cpp`/`pwm_input.cpp` are `volatile`; when adding fields shared between an ISR and the main loop, keep that pattern (read/write as a single access, not read-modify-write, where the two can race).
- `Joystick.send()` (and `hid.SendReport()` underneath) returns `false` silently when the USB host isn't ready/connected — this is expected during enumeration or if unplugged, not an error condition to "fix" by retrying or logging.
- HID reports are sent at a fixed ~100Hz cadence in the main loop (`now - lastHidMs >= 10`), independent of how often each RC decoder actually gets fresh data.
