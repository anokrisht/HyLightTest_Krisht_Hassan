# Hardware Test Procedures

1) CAN ID detection
- Configure `PB3..PB5` logic pins (pull-down or pull-up) per desired ID bits.
- Power cycle MCU and monitor CAN bus for ID `0x200 + value read from pins`.

2) Sensor switching and ΔP verification
- Connect BMP280 sensors to TCA9548A channels 0..5.
- Observe that each sensor responds with ID 0x58 (BMP280) on I2C.
- Apply a known pressure difference to pairs (0/1,2/3,4/5) and verify ΔP values in diagnostics.

3) Fan control
- Use UART control commands (COBS encoded) to send 0x01/0x02/0x03 and observe fan behavior.
- Use I2C monitor to verify MAX6650 registers if possible.

4) Fault tests
- Disconnect a sensor and ensure status flag bit4 is set and ΔP for related pair is invalid.
- Disconnect fan or block it; expect RPM=0 and fan fault flag set.
