/* app.c - application layer: initialize drivers and services and provide poll */
#include "app.h"
#include "sensors.h"
#include "fan.h"
#include "comms.h"

void app_init(void)
{
    sensors_init();
    fan_init();
    comms_init();
}

void app_poll(void)
{
    sensors_poll();
    fan_poll();
    comms_poll();
}
