/* Host unit tests for Core/Lib/utils.c - using minimal Unity-compatible helpers */
#include "../Core/Lib/Inc/utils.h"
#include "unity/unity.h"
#include <string.h>

void test_crc_known(void)
{
    const uint8_t v[] = "123456789";
    uint16_t crc = crc16_ccitt(v, sizeof(v)-1);
    TEST_ASSERT_EQUAL_INT(0x29B1, crc);
}

void test_cobs_roundtrip(void)
{
    uint8_t in[] = {0x11, 0x00, 0x22, 0x33, 0x00, 0x44};
    uint8_t enc[32];
    uint8_t dec[32];
    size_t elen = cobs_encode(in, sizeof(in), enc);
    size_t dlen = cobs_decode(enc, elen, dec);
    TEST_ASSERT_EQUAL_INT((int)sizeof(in), (int)dlen);
    for (size_t i = 0; i < dlen; ++i) TEST_ASSERT_EQUAL_INT(in[i], dec[i]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_known);
    RUN_TEST(test_cobs_roundtrip);
    UNITY_END();
    return 0;
}
