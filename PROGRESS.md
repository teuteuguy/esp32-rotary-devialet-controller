# Project Progress — ESP32 Rotary Devialet Controller

Last updated: 2026-05-03, Asia/Singapore

## Repository

- GitHub: `git@github.com:teuteuguy/esp32-rotary-devialet-controller.git`
- Local checkout: `/home/pi/openclaw-workspaces/main/esp32-rotary-devialet-controller`
- Current branch: `main`
- Initial pushed commit: `1091cc5 Initial portable ESP32 Devialet controller scaffold`

## Product goal

Build a small ESP32-based rotary dial + display device for Devialet Phantom control.

Core user-facing capabilities:

- View current Devialet source
- View current Devialet volume
- Increase/decrease volume via rotary encoder
- Change source from the device
- Eventually support different ESP32 dial/display hardware platforms, not just M5Stack Dial

## Hardware direction

First target hardware:

- M5Stack Dial
- ESP32-S3
- round 240x240 LCD
- rotary encoder
- touch/button/buzzer

Important design constraint:

- Keep the project portable.
- Do not bake M5Stack Dial assumptions into app/control logic.
- Use hardware abstraction interfaces so other ESP32 dial + display platforms can be added later.

## Chosen stack

Initial recommendation/decision:

- PlatformIO project
- Arduino framework first
- C++ firmware

Reasoning:

- M5Stack Dial library support is Arduino-first.
- Devialet control is simple LAN HTTP JSON; ESP-IDF would be overkill for MVP.
- PlatformIO gives reproducible builds and better project hygiene than Arduino IDE.
- ESP-IDF can be considered later for advanced OTA, provisioning, or power management.

## Current repository scaffold

Created and pushed:

```text
README.md
PROGRESS.md
platformio.ini
docs/hardware-porting.md
include/app/Controller.h
include/devialet/DevialetClient.h
include/hardware/Hardware.h
include/platforms/M5DialHardware.h
src/app/Controller.cpp
src/devialet/DevialetClient.cpp
src/main.cpp
src/platforms/m5dial/M5DialHardware.cpp
```

Architecture intent:

```text
app/          portable controller state/input handling
devialet/     Devialet IP Control client
hardware/     generic display/input interfaces
platforms/    hardware adapters, starting with M5Dial
```

## Devialet API notes

Known useful endpoints from existing OpenClaw `devialet` skill/reference:

- Discovery: mDNS `_http._tcp`, TXT includes `manufacturer=Devialet`, `ipControlVersion=1`, `path=/ipcontrol/v1`
- Base URL: `http://<ip>:<port><path>`
- Current source: `GET /groups/current/sources/current`
- Source list: `GET /groups/current/sources`
- Volume: `GET /systems/current/sources/current/soundControl/volume`
- Volume up: `POST /systems/current/sources/current/soundControl/volumeUp`
- Volume down: `POST /systems/current/sources/current/soundControl/volumeDown`
- Set volume: `POST /systems/current/sources/current/soundControl/volume` with `{ "volume": <0-100> }`
- Play/select source: `POST /groups/current/sources/{sourceId}/playback/play`
- Mute/unmute: `POST /groups/current/sources/current/playback/mute|unmute`

## Current status

- M5Stack Dial detected on `/dev/ttyACM0` as Espressif USB JTAG/serial, MAC `24:58:7c:53:d6:14`.
- PlatformIO installed in local venv: `/home/pi/.platformio-venv/bin/pio`.
- Firmware builds successfully for `env:m5dial`.
- Firmware uploaded successfully to `/dev/ttyACM0`.
- Clean Wi-Fi provisioning implemented with WiFiManager captive portal, SSID `Devialet Dial Setup`.
- Optional Devialet host/port/path can be configured in the portal and is persisted in ESP32 Preferences.

## Current limitations / not yet verified

- mDNS discovery implementation is only a first-pass placeholder; should be hardened after testing.
- Devialet JSON field names should be verified against real device responses.
- Source switching UI is not implemented yet; current scaffold has volume + mute/status concepts only.
- Captive portal must be verified by Tim from phone/laptop: connect to SSID `Devialet Dial Setup`, open `http://192.168.4.1` if needed.
- Serial monitor produced no useful runtime logs after upload; likely missed early boot logs or app is waiting in portal. Add more visible display states if needed.

## What Tim needs to provide next

Minimum needed for the next implementation/testing pass:

1. Plug M5Stack Dial into the Pi via USB-C.
2. Confirm whether I may install PlatformIO if it is still missing.
3. Provide Wi-Fi configuration approach:
   - either temporary compile-time SSID/password,
   - or create a local ignored `secrets.ini`,
   - or ask me to implement captive portal/provisioning first.
4. Confirm Devialet target strategy:
   - use mDNS discovery first,
   - or provide fixed Devialet IP/port/path for faster MVP testing.

Likely optional but useful:

- Which Devialet room/system to control if multiple are found.
- Desired knob behavior:
  - one detent = Devialet volumeUp/volumeDown command, or
  - local target volume with debounced setVolume.
- Desired button behavior:
  - short press mute/unmute,
  - long press source menu.

## Next plan

1. Tim connects to `Devialet Dial Setup` captive portal and enters Wi-Fi details.
2. Verify M5Dial display/encoder/button APIs on real hardware.
3. Verify Devialet endpoint responses against local speakers.
4. Replace placeholder mDNS with robust TXT-based discovery if available in ESP32 Arduino APIs.
5. Build MVP UI:
   - boot/connect screen
   - current source/volume screen
   - volume adjustment feedback
   - mute indication
6. Add source list + selection UI.
7. Add long-press reset credentials/config gesture.
8. Commit each working milestone.

## Resume commands

```bash
cd /home/pi/openclaw-workspaces/main/esp32-rotary-devialet-controller
git status --short --branch
git log --oneline -5
```

PlatformIO venv commands:

```bash
/home/pi/.platformio-venv/bin/pio run
/home/pi/.platformio-venv/bin/pio device list
/home/pi/.platformio-venv/bin/pio run -t upload --upload-port /dev/ttyACM0
/home/pi/.platformio-venv/bin/pio device monitor --port /dev/ttyACM0 --baud 115200
```
