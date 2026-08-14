/* utils.h - CRC16 and COBS utilities (library) */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len);
size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out);
size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out);

#endif // UTILS_H
