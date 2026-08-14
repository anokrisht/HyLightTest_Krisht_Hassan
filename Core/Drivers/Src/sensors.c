/* sensors.c - simple BMP280 reader via TCA9548A (driver) */
#include "sensors.h"
#include "main.h"
#include "utils.h"
#include <string.h>

/* Assumptions: I2C1 is used. TCA9548A at 0x70. BMP280 at 0x76 on each channel. */
static sensors_t s = {0};

/* Use the I2C handle initialized by CubeMX in main.c */
extern I2C_HandleTypeDef hi2c1;

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
    /* I2C1 should be initialized by CubeMX-generated MX_I2C1_Init() in main.c */
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

    /* compute differential pressures per spec: ΔP1 = sensor1 - sensor0, ΔP2 = sensor3 - sensor2, ΔP3 = sensor5 - sensor4 */
    for (int i = 0; i < 3; ++i) {
        int a = i*2, b = i*2+1; /* a=0,b=1 for i=0 */
        if (s.present[a] && s.present[b]) {
            /* spec says sensor1 - sensor0, so use b - a */
            s.dp[i] = s.pressure_pa[b] - s.pressure_pa[a];
        } else {
            s.dp[i] = 0.0f; /* indicate invalid */
        }
    }
}

const sensors_t* sensors_get(void)
{
    return &s;
}

uint8_t sensors_status_flags(void)
{
    uint8_t flags = 0;
    /* bit1..3: dp valid flags for ΔP1..3 (use bits 1-3) */
    for (int i = 0; i < 3; ++i) {
        int a = i*2, b = i*2+1;
        if (s.present[a] && s.present[b]) flags |= (1 << (i+1));
    }
    /* bit4: sensor comm fault if any sensor missing */
    for (int i = 0; i < SENSORS_COUNT; ++i) {
        if (!s.present[i]) { flags |= (1<<4); break; }
    }
    return flags;
}
