#include <stdio.h>
#include <string.h>
#include "../Core/Inc/utils.h"

int main(void)
{
    const uint8_t test[] = {1,2,3,0,4,5};
    uint8_t enc[32];
    uint8_t dec[32];
    size_t el = cobs_encode(test, sizeof(test), enc);
    size_t dl = cobs_decode(enc, el, dec);
    if (dl != sizeof(test) || memcmp(test, dec, dl) != 0) {
        printf("COBS test failed\n");
        return 1;
    }
    uint16_t c = crc16_ccitt((const uint8_t*)"123456", 6);
    printf("CRC16(\"123456\")=0x%04X\n", c);
    printf("utils tests passed\n");
    return 0;
}
