/* Simple host test for BMP280 compensation math (standalone) */
#include <stdio.h>
#include <stdint.h>

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
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)cal->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal->dig_P7) << 4);
    return (uint32_t)(p >> 8);
}

int main(void)
{
    bmp280_calib_t cal = {
        .dig_T1 = 27504,
        .dig_T2 = 26435,
        .dig_T3 = -1000,
        .dig_P1 = 36477,
        .dig_P2 = -10685,
        .dig_P3 = 3024,
        .dig_P4 = 2855,
        .dig_P5 = 140,
        .dig_P6 = -7,
        .dig_P7 = 15500,
        .dig_P8 = -14600,
        .dig_P9 = 6000
    };
    int32_t raw_t = 519888; /* example raw values */
    int32_t raw_p = 415148;
    int32_t tfine = 0;
    int32_t T = bmp280_compensate_T(&cal, raw_t, &tfine);
    uint32_t P = bmp280_compensate_P(&cal, raw_p, tfine);
    printf("Compensated T (x0.01 C): %ld, t_fine: %ld\n", (long)T, (long)tfine);
    printf("Compensated P (Pa): %lu\n", (unsigned long)P);
    return (P == 0) ? 1 : 0;
}
