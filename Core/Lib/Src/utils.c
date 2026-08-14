/* utils.c - CRC16 and COBS utilities (library) */
#include "utils.h"

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

size_t cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out)
{
    const uint8_t *end = in + in_len;
    uint8_t *out_start = out;
    uint8_t *code_ptr = out++;
    uint8_t code = 1;

    while (in < end) {
        if (*in == 0) {
            *code_ptr = code;
            code_ptr = out++;
            code = 1;
            in++;
        } else {
            *out++ = *in++;
            code++;
            if (code == 0xFF) {
                *code_ptr = code;
                code_ptr = out++;
                code = 1;
            }
        }
    }
    *code_ptr = code;
    return (size_t)(out - out_start);
}

size_t cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out)
{
    const uint8_t *end = in + in_len;
    size_t out_len = 0;

    while (in < end) {
        uint8_t code = *in++;
        if (code == 0) return 0; /* invalid */
        uint8_t i = 1;
        for (; i < code; ++i) {
            if (in >= end) return 0;
            out[out_len++] = *in++;
        }
        if (code < 0xFF && in < end) {
            out[out_len++] = 0;
        }
    }
    return out_len;
}
