# ESP32 Rotary Devialet Controller

Portable ESP32 firmware for a rotary dial + display controller for Devialet Phantom speakers using the local Devialet IP Control API.

The first hardware target is the **M5Stack Dial**, but the project is intentionally structured so other ESP32 dial/display devices can be supported later.

## Goals

- View current Devialet source
- View current volume
- Increase/decrease volume with a rotary encoder
- Switch source from the device UI
- Support multiple hardware platforms via a small hardware abstraction layer

## Recommended stack

- **PlatformIO** project
- **Arduino framework** initially, for fast hardware/library support
- Portable C++ core logic separated from board-specific drivers

ESP-IDF can be introduced later if we need deeper Wi-Fi, provisioning, OTA, or power-management control.

## Current status

Initial scaffold only. Hardware support begins with `m5dial`, but the core Devialet client and controller state machine should remain platform-independent.

## Repository layout

```text
include/
  app/                 # portable app/controller interfaces
  devialet/            # Devialet IP Control client
  hardware/            # abstract dial/display interfaces
  platforms/           # board-specific adapters
src/
  app/                 # app/controller implementation
  devialet/            # HTTP/mDNS client implementation
  platforms/m5dial/    # M5Stack Dial hardware adapter
  main.cpp             # firmware entry point
platformio.ini         # build environments
```

## Devialet API notes

Devialet Phantom DOS >= 2.14 exposes unauthenticated HTTP endpoints on the local network:

- Discover via mDNS `_http._tcp` entries with `manufacturer=Devialet` and `ipControlVersion=1`
- API prefix from TXT `path`, usually `/ipcontrol/v1`
- Current source: `GET /groups/current/sources/current`
- Source list: `GET /groups/current/sources`
- Volume: `GET /systems/current/sources/current/soundControl/volume`
- Volume up/down:
  - `POST /systems/current/sources/current/soundControl/volumeUp`
  - `POST /systems/current/sources/current/soundControl/volumeDown`
- Select/play source:
  - `POST /groups/current/sources/{sourceId}/playback/play`

## First target: M5Stack Dial

M5Dial gives us:

- ESP32-S3 Wi‑Fi MCU
- 1.28" 240x240 round TFT
- rotary encoder
- touch screen
- button/buzzer

The M5Dial adapter should translate hardware events into generic app events:

- encoder delta
- button press/long press
- touch gesture/tap
- display draw calls

## Provisioning

First-run Wi-Fi setup uses a captive portal.

If no saved Wi-Fi credentials exist, connect to:

```text
Devialet Dial Setup
```

Then open `http://192.168.4.1` if the captive portal does not appear automatically.

The portal also accepts an optional static Devialet endpoint. Leave it blank to try mDNS discovery.

## Planned MVP

1. Wi‑Fi captive portal provisioning
2. mDNS Devialet discovery with static-IP fallback
3. Display current volume/source
4. Rotary volume up/down
5. Source menu + source switching
6. OTA/config polish
