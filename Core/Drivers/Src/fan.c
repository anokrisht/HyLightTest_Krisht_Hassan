/* fan.c - MAX6650 minimal driver (detect RPM, set PWM) */
#include "fan.h"
#include "main.h"

static fan_t f = {0};
/* Use CubeMX I2C handle from main.c */
extern I2C_HandleTypeDef hi2c1; /* reuse I2C1 initialized by MX_I2C1_Init */
static fan_mode_t mode = FAN_MODE_AUTO;

fan_mode_t fan_get_mode(void) { return mode; }

void fan_set_mode(fan_mode_t m) { mode = m; }

void fan_init(void)
{
    /* assume I2C1 already initialised by sensors_init; mark present if device ACKs */
    f.present = (HAL_I2C_IsDeviceReady(&hi2c1, (0x2F<<1), 3, 50) == HAL_OK);
    f.pwm = 0;
    f.rpm = 0;
    f.fault = false;
}

void fan_set_pwm(uint8_t pwm)
{
    f.pwm = pwm;
    if (!f.present) return;
    uint8_t data[2];
    data[0] = 0x01; /* register: FAN speed command (placeholder) */
    data[1] = pwm;
    HAL_I2C_Master_Transmit(&hi2c1, (0x2F<<1), data, 2, 100);
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
    uint8_t reg = 0x00;
    uint8_t buf[2] = {0};
    if (HAL_I2C_Master_Transmit(&hi2c1, (0x2F<<1), &reg, 1, 100) != HAL_OK ||
        HAL_I2C_Master_Receive(&hi2c1, (0x2F<<1), buf, 2, 100) != HAL_OK) {
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
