# Module Methods and Usage

This document lists public methods across drivers and services, and brief usage notes.

`app.h`
- `void app_init(void)` — Initialize sensors, fan and comms.
- `void app_poll(void)` — Called periodically (~500 ms) to sample sensors, update fan and transmit diagnostics.

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
- `void comms_init(void)` — Initialize UART and CAN; reads CAN ID from GPIO PC0..PC2.
- `void comms_poll(void)` — Periodic transmit of diagnostics and UART frame processing.
- `void comms_send_diagnostics(const void *payload, uint16_t len)` — Low-level CAN TX helper.

`utils.h`
- `uint16_t crc16_ccitt(const uint8_t *data, size_t len)` — CRC-16-CCITT over a buffer.
- `size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out)` — COBS encode.
- `size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out)` — COBS decode; returns decoded length or 0 on error.
