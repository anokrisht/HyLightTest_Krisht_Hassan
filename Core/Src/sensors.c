/* sensors.c - simple BMP280 reader via TCA9548A */
#include "sensors.h"
#include "main.h"
#include "utils.h"
#include <string.h>

/* Assumptions: I2C1 is used. TCA9548A at 0x70. BMP280 at 0x76 on each channel. */
static sensors_t s = {0};

static I2C_HandleTypeDef hi2c1;

/* Minimal I2C init (user may change pins in board) */
static void I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* PB6 SCL, PB7 SDA typical - adjust if needed */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00707CBB; /* ~100kHz (approx) */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static bool tca_select_channel(uint8_t ch)
{
    if (ch >= 8) return false;
    uint8_t cmd = (1 << ch);
    if (HAL_I2C_Master_Transmit(&hi2c1, 0x70 << 1, &cmd, 1, 100) != HAL_OK) return false;
    return true;
}

static bool bmp280_read_raw(uint8_t channel, int32_t *out_raw)
{
    if (!tca_select_channel(channel)) return false;
    uint8_t id = 0;
    if (HAL_I2C_Mem_Read(&hi2c1, 0x76<<1, 0xD0, I2C_MEMADD_SIZE_8BIT, &id, 1, 100) != HAL_OK)
        return false;
    if (id != 0x58 && id != 0x60) return false; /* BMP280/other */

    uint8_t buf[3];
    if (HAL_I2C_Mem_Read(&hi2c1, 0x76<<1, 0xF7, I2C_MEMADD_SIZE_8BIT, buf, 3, 200) != HAL_OK)
        return false;
    int32_t raw = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)(buf[2] >> 4));
    *out_raw = raw;
    return true;
}

void sensors_init(void)
{
    memset(&s, 0, sizeof(s));
    I2C1_Init();
    for (int i = 0; i < SENSORS_COUNT; ++i) s.present[i] = false;
}

void sensors_poll(void)
{
    for (uint8_t ch = 0; ch < SENSORS_COUNT; ++ch) {
        int32_t raw;
        bool ok = bmp280_read_raw(ch, &raw);
        s.present[ch] = ok;
        if (ok) {
            s.raw_pressure[ch] = raw;
            /* crude conversion to Pa: raw / 256 * 100 (approx). Replace with proper compensation for accuracy. */
            s.pressure_pa[ch] = ((float)raw) / 256.0f;
        } else {
            s.pressure_pa[ch] = 0.0f;
        }
    }

    /* compute differential pressures: assume sensors 0/1 -> ΔP1, 2/3 -> ΔP2, 4/5 -> ΔP3 */
    for (int i = 0; i < 3; ++i) {
        int a = i*2, b = i*2+1;
        if (s.present[a] && s.present[b]) {
            s.dp[i] = s.pressure_pa[a] - s.pressure_pa[b];
        } else {
            s.dp[i] = 0.0f; /* indicate invalid */
        }
    }
}

const sensors_t* sensors_get(void)
{
    return &s;
}
