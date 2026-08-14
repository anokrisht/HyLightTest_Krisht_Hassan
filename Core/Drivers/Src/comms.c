/* comms.c - minimal CAN TX and UART COBS+CRC16 streaming (driver) */
#include "comms.h"
#include "main.h"
#include "utils.h"
#include "sensors.h"
#include "fan.h"
#include <string.h>

/* UART (USART2) and FDCAN1 handles */
static UART_HandleTypeDef huart2;
static FDCAN_HandleTypeDef hfdcan1;
static uint32_t can_id = 0x100;

static uint8_t uart_rx_buf[256];
static uint8_t uart_frame_buf[512];
static size_t uart_rx_len = 0;

/* Minimal UART init: PA2 Tx, PA3 Rx */
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* Minimal FDCAN init for TX-only */
static void FDCAN1_Init(void)
{
    __HAL_RCC_FDCAN_CLK_ENABLE();
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = DISABLE;
    hfdcan1.Init.NominalPrescaler = 1;
    hfdcan1.Init.NominalSyncJumpWidth = 1;
    hfdcan1.Init.NominalTimeSeg1 = 13;
    hfdcan1.Init.NominalTimeSeg2 = 2;
    hfdcan1.Init.DataPrescaler = 1;
    hfdcan1.Init.DataSyncJumpWidth = 1;
    hfdcan1.Init.DataTimeSeg1 = 1;
    hfdcan1.Init.DataTimeSeg2 = 1;
    HAL_FDCAN_Init(&hfdcan1);
}

static void read_can_id_from_gpio(void)
{
    /* Configure PC0..PC2 as inputs with pull-down and read a 3-bit ID */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    uint32_t v = 0;
    v |= (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0) == GPIO_PIN_SET) ? 1U : 0U;
    v |= (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1) == GPIO_PIN_SET) ? 2U : 0U;
    v |= (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2) == GPIO_PIN_SET) ? 4U : 0U;
    /* base ID configurable; use 0x200 + v as standard CAN ID */
    can_id = 0x200 + v;
}

void comms_init(void)
{
    UART2_Init();
    FDCAN1_Init();
    read_can_id_from_gpio();
    /* start UART receive in interrupt mode */
    HAL_UART_Receive_IT(&huart2, uart_rx_buf, 1);
}

/* Simple diagnostic CAN transmit: single frame with up to 8 bytes (truncated) */
void comms_send_diagnostics(const void *payload, uint16_t len)
{
    FDCAN_TxHeaderTypeDef txHeader = {0};
    txHeader.Identifier = can_id;
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    /* payload expected up to 12 bytes (we send 11) */
    txHeader.DataLength = FDCAN_DLC_BYTES_12;
    uint8_t data[12] = {0};
    memcpy(data, payload, (len>12)?12:len);
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, data);
}

/* UART receive IRQ handler: collect bytes, decode COBS frames with CRC16 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) return;
    uint8_t b = uart_rx_buf[0];
    if (uart_rx_len < sizeof(uart_frame_buf)) uart_frame_buf[uart_rx_len++] = b;
    /* Restart reception */
    HAL_UART_Receive_IT(&huart2, uart_rx_buf, 1);
}

static void process_uart_frames(void)
{
    /* look for 0x00 terminator (COBS frame delimiter) */
    size_t start = 0;
    for (size_t i = 0; i < uart_rx_len; ++i) {
        if (uart_frame_buf[i] == 0) {
            size_t frame_len = i - start;
            if (frame_len > 0) {
                uint8_t decoded[512];
                size_t dec_len = cobs_decode(&uart_frame_buf[start], frame_len, decoded);
                /* Expect control packet: 1 byte command + 2 bytes CRC = 3 bytes decoded */
                if (dec_len == 3) {
                    uint16_t recv_crc = (decoded[1] << 8) | decoded[2];
                    uint16_t calc = crc16_ccitt(decoded, 1);
                    if (calc == recv_crc) {
                        uint8_t cmd = decoded[0];
                        if (cmd == 0x01) {
                            fan_set_mode(FAN_MODE_FORCED_ON);
                        } else if (cmd == 0x02) {
                            fan_set_mode(FAN_MODE_FORCED_OFF);
                        } else if (cmd == 0x03) {
                            fan_set_mode(FAN_MODE_AUTO);
                        }
                    }
                    /* invalid CRC: discard silently */
                }
            }
            start = i+1;
        }
    }
    /* compact buffer */
    if (start > 0) {
        size_t remain = uart_rx_len - start;
        memmove(uart_frame_buf, &uart_frame_buf[start], remain);
        uart_rx_len = remain;
    }
}

void comms_poll(void)
{
    /* Send periodic diagnostics over CAN and UART */
    static uint32_t last_diag = 0;
    if (HAL_GetTick() - last_diag >= 500) {
        last_diag = HAL_GetTick();
        const sensors_t *s = sensors_get();
        const fan_t *fn = fan_get();
        /* Build full diagnostic payload per spec (11 bytes before COBS):
           ΔP1 int16, ΔP2 int16, ΔP3 int16, Fan RPM uint16, Status uint8, CRC16 uint16 */
        uint8_t payload[11];
        int16_t dp1 = (int16_t)(s->dp[0]);
        int16_t dp2 = (int16_t)(s->dp[1]);
        int16_t dp3 = (int16_t)(s->dp[2]);
        payload[0] = (dp1 >> 8) & 0xFF;
        payload[1] = dp1 & 0xFF;
        payload[2] = (dp2 >> 8) & 0xFF;
        payload[3] = dp2 & 0xFF;
        payload[4] = (dp3 >> 8) & 0xFF;
        payload[5] = dp3 & 0xFF;
        payload[6] = (fn->rpm >> 8) & 0xFF;
        payload[7] = fn->rpm & 0xFF;
        /* status flags: bit0 fan fault, bits1-3 dp valid, bit4 sensor comm fault */
        uint8_t status = 0;
        if (fn->fault) status |= 1<<0;
        status |= sensors_status_flags();
        payload[8] = status;
        /* CRC over first 9 bytes */
        uint16_t crc = crc16_ccitt(payload, 9);
        payload[9] = (crc >> 8) & 0xFF;
        payload[10] = crc & 0xFF;

        /* Send over CAN (will truncate/pad to 12 bytes) */
        comms_send_diagnostics(payload, sizeof(payload));

        /* Also stream over UART: COBS encode whole 11-byte payload then 0x00 delimiter */
        uint8_t enc[128];
        size_t elen = cobs_encode(payload, sizeof(payload), enc);
        HAL_UART_Transmit(&huart2, enc, elen, 200);
        uint8_t z = 0;
        HAL_UART_Transmit(&huart2, &z, 1, 50);
    }

    process_uart_frames();
}
