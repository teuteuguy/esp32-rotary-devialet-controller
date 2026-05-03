# Provisioning

The firmware uses WiFiManager for clean first-run Wi-Fi setup.

## First boot

If the device has no saved Wi-Fi credentials, it starts a captive portal access point:

```text
SSID: Devialet Dial Setup
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

A firmware UI gesture for resetting saved credentials is still TODO. During development, erase flash or add a temporary reset build.
