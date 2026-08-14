/* fan.h - MAX6650 fan controller interface (driver) */
#ifndef FAN_H
#define FAN_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool present;
    uint32_t rpm;
    uint8_t pwm; /* commanded PWM 0-255 */
    bool fault;
} fan_t;

typedef enum { FAN_MODE_AUTO = 0, FAN_MODE_FORCED_ON, FAN_MODE_FORCED_OFF } fan_mode_t;

void fan_init(void);
void fan_poll(void);
void fan_set_pwm(uint8_t pwm);
void fan_set_mode(fan_mode_t mode);
fan_mode_t fan_get_mode(void);
const fan_t* fan_get(void);

#endif // FAN_H
