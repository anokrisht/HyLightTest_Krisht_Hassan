/* fan.h - MAX6650 fan controller interface */
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

void fan_init(void);
void fan_poll(void);
void fan_set_pwm(uint8_t pwm);
const fan_t* fan_get(void);

#endif // FAN_H
