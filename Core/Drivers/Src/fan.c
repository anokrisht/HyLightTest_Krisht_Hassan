/* fan.c - MAX6650 minimal driver (detect RPM, set PWM) */
#include "fan.h"
#include "main.h"

/* MAX6650 implementation guide:
    See Core/Docs/MAX6650.md for the required register map, I2C address, and init steps.
    The driver below provides configurable `#define`s that can be overridden at
    build time (e.g. -DMAX6650_I2C_ADDR=0xXX). Replace these with exact values
    from your board's MAX6650 datasheet before production.
*/
#ifndef MAX6650_I2C_ADDR
#define MAX6650_I2C_ADDR ((0x2F) << 1)
#endif
#ifndef MAX6650_REG_PWM
#define MAX6650_REG_PWM 0x01
#endif
#ifndef MAX6650_REG_RPM
#define MAX6650_REG_RPM 0x00
#endif

static fan_t f = {0};
/* Use CubeMX I2C handle from main.c */
extern I2C_HandleTypeDef hi2c2; /* use I2C2 initialized by MX_I2C2_Init to avoid SWD pin conflict */
static fan_mode_t mode = FAN_MODE_AUTO;

fan_mode_t fan_get_mode(void) { return mode; }

void fan_set_mode(fan_mode_t m) { mode = m; }

void fan_init(void)
{
    /* assume I2C2 already initialised by CubeMX; mark present if device ACKs */
    /* Probe device with a few retries to tolerate I2C timing/power-up */
    f.present = false;
    for (int i = 0; i < 3; ++i) {
        if (HAL_I2C_IsDeviceReady(&hi2c2, MAX6650_I2C_ADDR, 3, 50) == HAL_OK) { f.present = true; break; }
        HAL_Delay(10);
    }
    f.pwm = 0;
    f.rpm = 0;
    f.fault = !f.present;
}

void fan_set_pwm(uint8_t pwm)
{
    f.pwm = pwm;
    if (!f.present) return;
    uint8_t data[2];
    data[0] = MAX6650_REG_PWM; /* register: FAN speed command (placeholder) */
    data[1] = pwm;
    /* write with a couple retries */
    for (int i = 0; i < 2; ++i) {
        if (HAL_I2C_Master_Transmit(&hi2c2, MAX6650_I2C_ADDR, data, 2, 100) == HAL_OK) { f.fault = false; break; }
        f.fault = true;
        HAL_Delay(5);
    }
}

void fan_poll(void)
{
    if (!f.present) {
        f.fault = true;
        f.rpm = 0;
        return;
    }
    /* Apply forced modes */
    if (mode == FAN_MODE_FORCED_ON) {
        fan_set_pwm(255);
    } else if (mode == FAN_MODE_FORCED_OFF) {
        fan_set_pwm(0);
    }
    /* Read RPM register placeholder (2 bytes) with retries */
    uint8_t reg = MAX6650_REG_RPM;
    uint8_t buf[2] = {0};
    bool ok = false;
    for (int i = 0; i < 2; ++i) {
        if (HAL_I2C_Master_Transmit(&hi2c2, MAX6650_I2C_ADDR, &reg, 1, 100) == HAL_OK &&
            HAL_I2C_Master_Receive(&hi2c2, MAX6650_I2C_ADDR, buf, 2, 100) == HAL_OK) { ok = true; break; }
        HAL_Delay(5);
    }
    if (!ok) {
        f.fault = true;
        f.rpm = 0;
        return;
    }
    f.fault = false;
    f.rpm = ((uint32_t)buf[0] << 8) | buf[1];
    if (f.rpm == 0) f.fault = true; /* stall or disconnected */
}

const fan_t* fan_get(void)
{
    return &f;
}
