/* app.h - application layer API */
#ifndef APP_H
#define APP_H

#include <stdint.h>

/** Initialize application subsystems (drivers, services). */
void app_init(void);

/** Periodic poll to be called from main loop. */
void app_poll(void);

#endif // APP_H
