# Module Methods and Usage

This document lists public methods across drivers and services, and brief usage notes.

`app.h`
- `void app_init(void)` — Initialize sensors, fan and comms.
- `void app_poll(void)` — Cooperative scheduler entry; call frequently from `main()` and let internal gates run subsystems at different rates (comms ~100 ms, sensors/fan ~200 ms).

`sensors.h`
- `void sensors_init(void)` — Initialize I2C and prepare sensors.
- `void sensors_poll(void)` — Read all BMP280 sensors via TCA9548A and compute ΔP values.
- `const sensors_t* sensors_get(void)` — Access last measurements.
- `uint8_t sensors_status_flags(void)` — Get status bits: bits 1-3 ΔP valid, bit4 sensor comm fault.

`fan.h`
- `void fan_init(void)` — Detect fan controller on I2C.
- `void fan_poll(void)` — Read RPM and update fault state.
- `void fan_set_pwm(uint8_t pwm)` — Command PWM (0-255) to fan controller.
- `void fan_set_mode(fan_mode_t mode)` — Set `FAN_MODE_AUTO`, `FAN_MODE_FORCED_ON`, or `FAN_MODE_FORCED_OFF`.
- `fan_mode_t fan_get_mode(void)` — Query current mode.

`comms.h`
- `void comms_init(void)` — Initialize UART and CAN; reads CAN ID from GPIO pins configured in `MX_GPIO_Init` (default: `PB3..PB5`).
- `void comms_poll(void)` — Periodic transmit of diagnostics and UART frame processing.
- `void comms_send_diagnostics(const void *payload, uint16_t len)` — Low-level CAN TX helper.

`utils.h`
- `uint16_t crc16_ccitt(const uint8_t *data, size_t len)` — CRC-16-CCITT over a buffer.
- `size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out)` — COBS encode.
- `size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out)` — COBS decode; returns decoded length or 0 on error.

---

## API Reference (detailed)

All function prototypes below are the canonical declarations; include the corresponding headers in `Core/Drivers/Inc`, `Core/Lib/Inc`, or `Core/App/Inc`.

`app.h`
- `void app_init(void);`
	- Description: Initialize all subsystems (drivers, lib, comms). Safe to call once at boot.
	- Errors: none (internal subsystems set status flags on failures).
- `void app_poll(void);`
	- Description: Perform periodic tasks: sensor sampling, fan update, diagnostics transmit and UART frame processing.
	- Timing: Designed to be called in a main loop; internal timers gate 500 ms diagnostic transmit.

`sensors.h`
- `void sensors_init(void);`
	- Description: Initialize I²C (if driver not already initialized) and probe BMP280 devices through the TCA9548A multiplexer.
	- Behavior: marks sensor channels absent if probe fails.
	- Errors: none returned; check `sensors_status_flags()` to inspect presence/comm faults.
- Note: BMP280 calibration and full compensation is implemented. `sensors_init()` attempts to read calibration data from each BMP280 channel; `sensors_poll()` applies Bosch's temperature and pressure compensation formulas to produce calibrated pressure in Pascals.
 - Note: The firmware now uses I2C2 for sensor & fan peripherals to avoid SWD pin conflicts. Drivers (`sensors.c`, `fan.c`) reference the `hi2c2` handle initialized by `MX_I2C2_Init()`.
 - The CAN ID is read from GPIO pins `PB3..PB5` at startup (see `comms.c`). These pins are configured as inputs in `MX_GPIO_Init()`.
 - CAN transmission: diagnostics are sent using a purpose-built pair of Classic CAN frames (`can_id` and `can_id+1`). Frame A contains ΔP1, ΔP2, RPM, and status; Frame B contains ΔP3 and CRC.
- `void sensors_poll(void);`
	- Description: Read BMP280 raw registers on each configured channel and update stored measurements and ΔP values.
	- Notes: uses a simple scaling of raw pressure. Replace with full compensation using BMP280 calibration for production.
- `const sensors_t* sensors_get(void);`
	- Returns: pointer to a `sensors_t` struct containing fields:
		- `int16_t dp[3];` — computed differential pressures ΔP1..ΔP3 as signed 16-bit values (units: raw units scaled in driver).
		- `uint32_t timestamp_ms;` — HAL tick when last update occurred.
	- Ownership: pointer is valid until next `sensors_poll()`.
- `uint8_t sensors_status_flags(void);`
	- Bits definition (LSB=bit0):
		- bit0: reserved
		- bit1: ΔP1 valid (1=valid)
		- bit2: ΔP2 valid
		- bit3: ΔP3 valid
		- bit4: sensor communication fault (1=fault present)

`fan.h`
- `void fan_init(void);`
	- Description: Probe fan controller over I²C and initialize state machine.
- `void fan_poll(void);`
	- Description: Periodically read fan RPM and update `fan_t` fault flags. Non-blocking with I²C timeouts via HAL.
- `void fan_set_pwm(uint8_t pwm);`
	- Description: Command PWM duty via the fan controller (0..255). Implementation uses I²C write; clamped to supported range.
	- Errors: None returned; driver sets `fan_t.fault` on write failure.
- `void fan_set_mode(fan_mode_t mode);`
	- Description: Set operating mode. `fan_mode_t` enum values: `FAN_MODE_AUTO`, `FAN_MODE_FORCED_ON`, `FAN_MODE_FORCED_OFF`.
- `fan_mode_t fan_get_mode(void);`
	- Returns: current requested mode.
- `const fan_t* fan_get(void);`
	- Returns: pointer to `fan_t` structure with fields including `uint16_t rpm; bool fault; fan_mode_t mode;`.

`comms.h`
- `void comms_init(void);`
	- Description: Initialize UART and CAN peripherals. Reads CAN ID from GPIO pins `PB3..PB5` and configures TX queue.
- `void comms_poll(void);`
	- Description: Handle UART frame parsing (COBS + CRC) and periodically transmit diagnostic frames over UART and CAN.
	- Diagnostic payload (11 bytes before COBS):
		- bytes[0..1] ΔP1 (int16 BE),
		- bytes[2..3] ΔP2,
		- bytes[4..5] ΔP3,
		- bytes[6..7] Fan RPM (uint16 BE),
		- byte[8] Status flags,
		- bytes[9..10] CRC16 (BE) computed over bytes[0..8].
- `void comms_send_diagnostics(const void *payload, uint16_t len);`
	- Low-level helper to enqueue a CAN transmit and/or UART stream. Accepts payload up to 12 bytes; extra bytes truncated.

`utils.h`
- `uint16_t crc16_ccitt(const uint8_t *data, size_t len);`
	- CRC-16-CCITT with polynomial 0x1021 and initial value 0xFFFF. Returns 16-bit CRC.
	- Example: `crc16_ccitt((uint8_t*)"123456789", 9) == 0x29B1`.
- `size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out);`
	- COBS encoding as implemented in `Core/Lib/Src/utils.c`. Returns bytes written to `out`.
- `size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out);`
	- Returns decoded length, or `0` on error (malformed COBS input).

## Error Handling and Status Model
- Drivers set internal status flags and `fault` fields rather than returning complex error codes, to keep the main loop simple. Use `sensors_status_flags()` and `fan_get()` to observe health.
- Communications parsing discards invalid UART frames (bad COBS, bad CRC) and never raises exceptions or blocks the system.

## Example usage
- Initialization (in `main()`):

```c
app_init();
for (;;) {
		app_poll();
		HAL_Delay(10);
}
```

---

If you'd like, I can generate a machine-readable header summary (JSON) from these signatures for automated documentation generation.
