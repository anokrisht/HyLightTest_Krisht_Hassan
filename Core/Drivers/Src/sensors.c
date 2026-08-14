/* sensors.c - simple BMP280 reader via TCA9548A (driver) */
#include "sensors.h"
#include "main.h"
#include "utils.h"
#include <string.h>

/* Assumptions: I2C2 is used. TCA9548A at 0x70. BMP280 at 0x76 on each channel. */
static sensors_t s = {0};

/* Use the I2C handle initialized by CubeMX in main.c */
extern I2C_HandleTypeDef hi2c2;

/* BMP280 calibration parameters */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

static bmp280_calib_t bmp_cal[SENSORS_COUNT];
static bool bmp_cal_valid[SENSORS_COUNT] = {0};

static bool tca_select_channel(uint8_t ch)
{
    if (ch >= 8) return false;
    uint8_t cmd = (1 << ch);
    if (HAL_I2C_Master_Transmit(&hi2c2, 0x70 << 1, &cmd, 1, 100) != HAL_OK) return false;
    return true;
}



static bool bmp280_read_temp_and_press_raw(uint8_t channel, int32_t *raw_temp, int32_t *raw_press)
{
    if (!tca_select_channel(channel)) return false;
    uint8_t id = 0;
    if (HAL_I2C_Mem_Read(&hi2c2, 0x76<<1, 0xD0, I2C_MEMADD_SIZE_8BIT, &id, 1, 100) != HAL_OK)
        return false;
    if (id != 0x58 && id != 0x60) return false;

    uint8_t buf[6];
    /* Read pressure (3 bytes) then temperature (3 bytes) starting at 0xF7 */
    if (HAL_I2C_Mem_Read(&hi2c2, 0x76<<1, 0xF7, I2C_MEMADD_SIZE_8BIT, buf, 6, 200) != HAL_OK)
        return false;
    int32_t raw_p = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)(buf[2] >> 4));
    int32_t raw_t = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)(buf[5] >> 4));
    *raw_temp = raw_t;
    *raw_press = raw_p;
    return true;
}

static bool bmp280_read_calibration(uint8_t channel)
{
    /* select channel then read 24 bytes of calibration from 0x88 */
    if (!tca_select_channel(channel)) return false;
    uint8_t calib[24];
    if (HAL_I2C_Mem_Read(&hi2c2, 0x76<<1, 0x88, I2C_MEMADD_SIZE_8BIT, calib, 24, 200) != HAL_OK) return false;
    bmp_cal[channel].dig_T1 = (uint16_t)calib[0] | ((uint16_t)calib[1] << 8);
    bmp_cal[channel].dig_T2 = (int16_t)calib[2] | ((int16_t)calib[3] << 8);
    bmp_cal[channel].dig_T3 = (int16_t)calib[4] | ((int16_t)calib[5] << 8);
    bmp_cal[channel].dig_P1 = (uint16_t)calib[6] | ((uint16_t)calib[7] << 8);
    bmp_cal[channel].dig_P2 = (int16_t)calib[8] | ((int16_t)calib[9] << 8);
    bmp_cal[channel].dig_P3 = (int16_t)calib[10] | ((int16_t)calib[11] << 8);
    bmp_cal[channel].dig_P4 = (int16_t)calib[12] | ((int16_t)calib[13] << 8);
    bmp_cal[channel].dig_P5 = (int16_t)calib[14] | ((int16_t)calib[15] << 8);
    bmp_cal[channel].dig_P6 = (int16_t)calib[16] | ((int16_t)calib[17] << 8);
    bmp_cal[channel].dig_P7 = (int16_t)calib[18] | ((int16_t)calib[19] << 8);
    bmp_cal[channel].dig_P8 = (int16_t)calib[20] | ((int16_t)calib[21] << 8);
    bmp_cal[channel].dig_P9 = (int16_t)calib[22] | ((int16_t)calib[23] << 8);
    return true;
}

/* Compensation per BMP280 datasheet. Returns temperature *100 in degC (not used) and sets t_fine. */
static int32_t bmp280_compensate_T(const bmp280_calib_t *cal, int32_t adc_T, int32_t *t_fine_out)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)cal->dig_T1 << 1))) * ((int32_t)cal->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)cal->dig_T1)) * ((adc_T >> 4) - ((int32_t)cal->dig_T1))) >> 12) * ((int32_t)cal->dig_T3)) >> 14;
    *t_fine_out = var1 + var2;
    T = (*t_fine_out * 5 + 128) >> 8;
    return T;
}

static uint32_t bmp280_compensate_P(const bmp280_calib_t *cal, int32_t adc_P, int32_t t_fine_in)
{
    int64_t var1, var2, p;
    int32_t tfin = t_fine_in;
    var1 = ((int64_t)tfin) - 128000;
    var2 = var1 * var1 * (int64_t)cal->dig_P6;
    var2 = var2 + ((var1 * (int64_t)cal->dig_P5) << 17);
    var2 = var2 + (((int64_t)cal->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal->dig_P3) >> 8) + ((var1 * (int64_t)cal->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)cal->dig_P1)) >> 33;
    if (var1 == 0) return 0; /* avoid division by zero */
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)cal->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal->dig_P7) << 4);
    /* p is Q24.8 (pressure * 256), convert to Pa by >>8 */
    return (uint32_t)(p >> 8);
}

void sensors_init(void)
{
    memset(&s, 0, sizeof(s));
    /* I2C2 should be initialized by CubeMX-generated MX_I2C2_Init() in main.c */
    for (int i = 0; i < SENSORS_COUNT; ++i) {
        s.present[i] = false;
        bmp_cal_valid[i] = false;
        /* select channel and explicitly configure BMP280 ctrl_meas (oversampling + normal mode)
           so the sensor runs continuous measurements. Then read calibration. */
        if (!tca_select_channel(i)) continue;
        /* ctrl_meas: osrs_t=1, osrs_p=1, mode=3 (normal) -> 0x27 */
        uint8_t ctrl_meas = 0x27;
        if (HAL_I2C_Mem_Write(&hi2c2, 0x76<<1, 0xF4, I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1, 200) != HAL_OK) {
            /* sensor write failed — leave as not present and continue */
            continue;
        }
        /* allow the sensor to start measurements and settle */
        HAL_Delay(10);
        /* attempt to read calibration with retries */
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (bmp280_read_calibration(i)) { bmp_cal_valid[i] = true; break; }
            HAL_Delay(5);
        }
    }
}

void sensors_poll(void)
{
    for (uint8_t ch = 0; ch < SENSORS_COUNT; ++ch) {
        int32_t raw_t = 0, raw_p = 0;
        bool ok = bmp280_read_temp_and_press_raw(ch, &raw_t, &raw_p);
        /* only mark present and compute compensated pressure when calibration is valid */
        if (ok && bmp_cal_valid[ch]) {
            s.present[ch] = true;
            s.raw_pressure[ch] = raw_p;
            int32_t local_tfine = 0;
            bmp280_compensate_T(&bmp_cal[ch], raw_t, &local_tfine);
            uint32_t press_pa = bmp280_compensate_P(&bmp_cal[ch], raw_p, local_tfine);
            s.pressure_pa[ch] = (float)press_pa;
        } else {
            s.present[ch] = false;
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
