# Hardware porting guide

The app should not know whether it is running on an M5Stack Dial, LilyGo T-Encoder, custom ESP32 board, or anything else.

To add a new hardware target:

1. Implement `hardware::Display`
2. Implement `hardware::Input`
3. Implement `hardware::Hardware`
4. Add a PlatformIO environment with a target macro, e.g. `TARGET_MY_BOARD=1`
5. Select that implementation in `src/main.cpp`

Required input events:

- encoder delta
- short button press
- long button press
- optional touch coordinates

Required display methods:

- boot message
- status screen
- error screen

Keep Devialet/network logic out of board adapters.
