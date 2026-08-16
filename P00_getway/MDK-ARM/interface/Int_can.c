#include "Int_can.h"

void Int_fdcan_init(void)
{
    FDCAN_FilterTypeDef fdcan_config = {0};
    fdcan_config.FilterConfig = FDCAN_FILTER_DISABLE;
    fdcan_config.FilterID1 = 0;
    fdcan_config.FilterID2 = 0;
    fdcan_config.FilterType = FDCAN_FILTER_MASK;
    fdcan_config.IdType = FDCAN_STANDARD_ID;
    fdcan_config.FilterIndex = 0;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &fdcan_config);

    HAL_FDCAN_Start(&hfdcan1);
}

void Int_fdcan_send(uint16_t id, uint8_t *data, uint8_t len)
{
    FDCAN_TxHeaderTypeDef tx_header = {0};
    tx_header.DataLength = len;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.MessageMarker = 0;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.Identifier = id;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, data);
}

void Int_fdcan_recv(can_recv_msg_t *msg, uint32_t *count)
{
    *count = HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0);
    for (uint32_t i = 0; i < *count; i++)
    {
        memset(&msg[i], 0, sizeof(can_recv_msg_t));
        HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &msg[i].rx_header, msg[i].data);
    }
}
