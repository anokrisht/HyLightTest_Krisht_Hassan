/* tests/test_fan.c - basic fan logic tests without HAL (pure logic) */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum { FAN_MODE_AUTO=0, FAN_MODE_FORCED_ON=1, FAN_MODE_FORCED_OFF=2 } fan_mode_t;

typedef struct {
    bool present;
    bool fault;
    uint16_t rpm;
    uint8_t pwm;
    fan_mode_t mode;
} fan_state_t;

void fan_set_mode(fan_state_t *s, fan_mode_t m) { s->mode = m; }

void fan_set_pwm(fan_state_t *s, uint8_t pwm) { s->pwm = pwm; }

/* Simulate a poll: apply modes and compute fault from rpm==0 */
void fan_poll_sim(fan_state_t *s) {
    if (!s->present) { s->fault = true; s->rpm = 0; return; }
    if (s->mode == FAN_MODE_FORCED_ON) {
        fan_set_pwm(s, 255);
    } else if (s->mode == FAN_MODE_FORCED_OFF) {
        fan_set_pwm(s, 0);
    }
    /* Fault detection: rpm==0 -> fault */
    if (s->rpm == 0) s->fault = true; else s->fault = false;
}

int main(void) {
    fan_state_t s = { .present = true, .fault = false, .rpm = 1000, .pwm = 128, .mode = FAN_MODE_AUTO };
    fan_set_mode(&s, FAN_MODE_FORCED_ON);
    fan_poll_sim(&s);
    if (s.pwm != 255) { printf("Forced on failed: pwm=%d\n", s.pwm); return 1; }

    fan_set_mode(&s, FAN_MODE_FORCED_OFF);
    fan_poll_sim(&s);
    if (s.pwm != 0) { printf("Forced off failed: pwm=%d\n", s.pwm); return 2; }

    /* Simulate stall */
    s.mode = FAN_MODE_AUTO; s.rpm = 0; s.pwm = 128; s.fault = false;
    fan_poll_sim(&s);
    if (!s.fault) { printf("Stall not detected\n"); return 3; }

    printf("fan logic tests passed\n");
    return 0;
}
