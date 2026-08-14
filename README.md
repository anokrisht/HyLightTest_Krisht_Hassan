# HyLightTest_Krisht_Hassan

Embedded firmware for the HyLighter Pressure Control System (STM32G431).

Overview
- Sensor drivers for six BMP280 via TCA9548A I2C multiplexer.
- Fan driver for MAX6650-like controller over I2C.
- Diagnostics over CAN (TX) and UART (COBS + CRC16).
- Application layer (`app.c`) keeps `main.c` minimal.

Build
Use the existing CMake toolchain to build for the target. Example (Linux/macOS):
```bash
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build -- -j4
```

Tests
- A host test for `utils` is provided under `tests/` and can be built with a native GCC.

Hardware tests
- See HOWTO-HARDWARE-TEST.md for step-by-step hardware validation.
