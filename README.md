# HyLightTest_Krisht_Hassan

Embedded firmware for the HyLighter Pressure Control System (STM32G431).

## Architecture Overview
- **Layers**
	- **Drivers (HAL wrappers)**: low-level I2C/CAN/UART access and sensor/fan drivers under [Core/Drivers](Core/Drivers/).
	- **Lib**: protocol helpers (COBS, CRC) under [Core/Lib](Core/Lib/).
	- **App (services)**: application logic, sampling, and fault handling under [Core/App](Core/App/).
	- **Entry point**: minimal `main.c` in [Core/Src/main.c](Core/Src/main.c#L1-L220) which calls `app_init()` and `app_poll()`.

## Logic flow
1. Boot → `main()` calls `app_init()`.
2. `app_init()` initializes drivers: sensors, fan, comms.
3. Periodic `app_poll()` (~500 ms):
	 - `sensors_poll()` reads six BMP280 sensors via the TCA9548A multiplexer and computes ΔP1..ΔP3.
	 - `fan_poll()` reads RPM and detects faults; modes can be `AUTO`, `FORCED_ON`, `FORCED_OFF`.
	 - `comms_poll()` packages diagnostics (ΔP1..3, RPM, status) into an 11-byte payload, appends CRC16, encodes with COBS, transmits over UART, and sends diagnostics over CAN (TX only). It also parses UART control packets (COBS + CRC) to change fan mode.

## Key files (links)
- Drivers: [Core/Drivers/](Core/Drivers/)
- Lib (utils): [Core/Lib/](Core/Lib/)
- App: [Core/App/](Core/App/)
- Main entry: [Core/Src/main.c](Core/Src/main.c#L1-L220)
- API reference: [Core/Docs/METHODS.md](Core/Docs/METHODS.md)
- Hardware test instructions: [HOWTO-HARDWARE-TEST.md](HOWTO-HARDWARE-TEST.md)
- Host tests: [tests/](tests/)

## Assumptions and simplifications
- Sensors: BMP280 devices are available at I2C address `0x76` on each TCA9548A channel; ID check accepts `0x58` or `0x60`.
- Fan controller: MAX6650 registers and address are approximated (placeholder `0x2F`); replace with exact datasheet registers for production.
- Pressure compensation: BMP280 calibration and full compensation formulas are implemented in `sensors.c`; this produces calibrated pressures in Pascals.
	Drivers use `I2C2` to avoid SWD pin conflicts — ensure `MX_I2C2_Init()` runs before `app_init()`.
- Timing: software uses a simple polling loop (~500 ms). A real system may require an RTOS or timer-driven architecture for tighter timing and concurrency.

## Steps to reproduce and test firmware

1) Native (host) tests for protocol helpers

```bash
cd tests
gcc -I../Core/Lib/Inc ../Core/Lib/Src/utils.c utils_test.c -o utils_test
./utils_test
```

2) Cross-build firmware (requires `cmake`, `ninja`, and `gcc-arm-none-eabi`)

```bash
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build -- -j4
```

3) Hardware tests (quick)
 - Configure CAN ID pins (default: `PA8..PA10`) and power-cycle to verify CAN ID read. See [HOWTO-HARDWARE-TEST.md](HOWTO-HARDWARE-TEST.md).
- Connect BMP280 sensors to TCA9548A channels 0..5 and verify ΔP outputs.
- Use UART to receive COBS-encoded diagnostic frames (11-byte payload before COBS) and to send control packets (1-byte command + CRC16, COBS-encoded):
	- `0x01` → Force fan ON
	- `0x02` → Force fan OFF
	- `0x03` → Return to automatic mode

## Next recommendations
- Replace placeholder registers/addresses in `Core/Drivers` with exact values from sensor and fan datasheets.
- Add unit tests (Unity) for drivers and services and run them in CI.
- Consider introducing a lightweight scheduler or RTOS for deterministic behavior.
