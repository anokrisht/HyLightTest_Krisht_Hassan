/* app.c - application layer: initialize drivers and services and provide poll */
#include "app.h"
#include "main.h"
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
    /* simple cooperative scheduler: run subsystems at different intervals
       - sensors: every 200 ms
       - fan: every 200 ms
       - comms: every 100 ms (keeps UART responsive) */
    static uint32_t last_sensors = 0;
    static uint32_t last_fan = 0;
    static uint32_t last_comms = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last_comms) >= 100) { last_comms = now; comms_poll(); }
    if ((now - last_sensors) >= 200) { last_sensors = now; sensors_poll(); }
    if ((now - last_fan) >= 200) { last_fan = now; fan_poll(); }
}
