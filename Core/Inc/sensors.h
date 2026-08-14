/* sensors.h - BMP280 via TCA9548A multiplexer */
#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

#define SENSORS_COUNT 6

typedef struct {
    bool present[SENSORS_COUNT];
    int32_t raw_pressure[SENSORS_COUNT];
    float pressure_pa[SENSORS_COUNT];
    float dp[3]; /* ΔP1..3 */
} sensors_t;

void sensors_init(void);
void sensors_poll(void);
const sensors_t* sensors_get(void);
uint8_t sensors_status_flags(void);

#endif // SENSORS_H
