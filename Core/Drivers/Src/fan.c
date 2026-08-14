/* fan.c - MAX6650 minimal driver (detect RPM, set PWM) */
#include "fan.h"
#include "main.h"

/* NOTE: MAX6650 register map and I2C address must be verified against the datasheet.
    The defines below are placeholders and should be updated before production. */
#define MAX6650_I2C_ADDR ((0x2F) << 1) /* TODO: confirm 7-bit address from datasheet */
#define MAX6650_REG_PWM 0x01 /* placeholder register for PWM command */
#define MAX6650_REG_RPM 0x00 /* placeholder register to read RPM (2 bytes) */

static fan_t f = {0};
/* Use CubeMX I2C handle from main.c */
extern I2C_HandleTypeDef hi2c2; /* use I2C2 initialized by MX_I2C2_Init to avoid SWD pin conflict */
static fan_mode_t mode = FAN_MODE_AUTO;

fan_mode_t fan_get_mode(void) { return mode; }

void fan_set_mode(fan_mode_t m) { mode = m; }

void fan_init(void)
{
    /* assume I2C2 already initialised by CubeMX; mark present if device ACKs */
    if (HAL_I2C_IsDeviceReady(&hi2c2, MAX6650_I2C_ADDR, 3, 50) == HAL_OK) {
        f.present = true;
    } else {
        f.present = false;
        f.fault = true;
    }
    f.pwm = 0;
    f.rpm = 0;
    f.fault = false;
}

void fan_set_pwm(uint8_t pwm)
{
    f.pwm = pwm;
    if (!f.present) return;
    uint8_t data[2];
    data[0] = MAX6650_REG_PWM; /* register: FAN speed command (placeholder) */
    data[1] = pwm;
    if (HAL_I2C_Master_Transmit(&hi2c2, MAX6650_I2C_ADDR, data, 2, 100) != HAL_OK) {
        f.fault = true;
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
    /* Read RPM register placeholder (2 bytes) */
    uint8_t reg = MAX6650_REG_RPM;
    uint8_t buf[2] = {0};
    if (HAL_I2C_Master_Transmit(&hi2c2, MAX6650_I2C_ADDR, &reg, 1, 100) != HAL_OK ||
        HAL_I2C_Master_Receive(&hi2c2, MAX6650_I2C_ADDR, buf, 2, 100) != HAL_OK) {
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
