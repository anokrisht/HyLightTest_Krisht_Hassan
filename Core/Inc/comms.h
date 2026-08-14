/* comms.h - CAN TX and UART (COBS + CRC16) */
#ifndef COMMS_H
#define COMMS_H

#include <stdint.h>

void comms_init(void);
void comms_poll(void);
void comms_send_diagnostics(const void *payload, uint16_t len);

#endif // COMMS_H
