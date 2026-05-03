# USB serial logs and control

The firmware exposes logs and a small control shell over USB CDC serial at 115200 baud.

Use PlatformIO monitor:

```bash
/home/pi/.platformio-venv/bin/pio device monitor --port /dev/ttyACM0 --baud 115200
```

Commands:

```text
help
endpoint
discover
device
status
sources
volume_up
volume_down
volume <0-100>
set_endpoint <host> [port] [path]
```

Examples:

```text
status
volume 35
set_endpoint 192.168.68.176 80 /ipcontrol/v1
```

Discovery is probe-based: the device enumerates `_http._tcp` mDNS candidates, then probes `/ipcontrol/v1/devices/current` and selects only a candidate that looks like Devialet IP Control and exposes controllable current volume/source endpoints.

It deliberately does **not** rely on user-editable friendly service names such as “Living Room” or “Phantom 108 dB Right”.
