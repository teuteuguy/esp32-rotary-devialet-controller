# Provisioning

The firmware uses a small built-in captive portal for clean first-run Wi-Fi setup.

## First boot

If the device has no saved Wi-Fi credentials, it starts a captive portal access point:

```text
SSID: DevialetDialSetup
Password: devialet
```

Connect to that network from a phone/laptop. The captive portal should open automatically; if it does not, browse to:

```text
http://192.168.4.1
```

Configure:

- Wi-Fi SSID/password
- Optional Devialet IP/host fallback
- Optional Devialet port, default `80`
- Optional Devialet API path, default `/ipcontrol/v1`

If Devialet host is left blank, firmware will try mDNS discovery after joining Wi-Fi.

## Resetting credentials

On boot, the display shows `Hold to reset Wi-Fi` for about 3 seconds. Hold the screen button during this window to clear saved Wi-Fi/config and restart into provisioning.

During development, flash/NVS can also be erased from PlatformIO:

```bash
/home/pi/.platformio-venv/bin/pio run -t erase --upload-port /dev/ttyACM0
/home/pi/.platformio-venv/bin/pio run -t upload --upload-port /dev/ttyACM0
```

## Display sleep

After Wi-Fi onboarding, the display turns off after 30 seconds of no touch/button/encoder activity. Touching the screen, pressing the button, or turning the dial wakes the display. The first wake interaction is consumed so an accidental wake touch does not also change volume/source.

During Wi-Fi onboarding/setup screens, the display remains on.
