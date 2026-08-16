#include "Int_can.h"

void Int_CAN_init(void)
{
    // 1. 配置过滤器
    CAN_FilterTypeDef filter_config = {0};
    filter_config.FilterBank = 0;                      // 过滤器编号，一共14个过滤器，0-13
    filter_config.FilterMode = CAN_FILTERMODE_IDMASK;  // 过滤器模式，掩码模式，模糊匹配
    filter_config.FilterScale = CAN_FILTERSCALE_32BIT; // 过滤器位数，32位，16位，8位

    // 使用哪个接收队列
    filter_config.FilterFIFOAssignment = CAN_RX_FIFO0;
    // 只要掩码都是0，表示ID不需要匹配上，掩码哪一位是1，表示哪一位需要匹配上。高位在前
    filter_config.FilterMaskIdHigh = 0;
    filter_config.FilterMaskIdLow = 0;
    // 填写寄存器
    filter_config.FilterIdHigh = 0;
    filter_config.FilterIdLow = 0;
    // 过滤器使能，0-1
    filter_config.FilterActivation = CAN_FILTER_ENABLE;

    HAL_CAN_ConfigFilter(&hcan, &filter_config);

    HAL_CAN_Start(&hcan);
}

/**
 * @brief 发送CAN消息
 *
 * @param id CAN ID，标准ID是11位，扩展ID是29位
 * @param data 数据指针
 * @param len 数据长度
 */
void Int_CAN_send(uint16_t id, uint8_t *data, uint8_t len)
{
    // 发送消息前，要检查是否有空闲的发送邮箱
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
    {
    }

    if (len > 8)
    {
        printf("len is too long, max is 8\n");
        return;
    }

    CAN_TxHeaderTypeDef tx_header = {0};
    tx_header.StdId = id;         // ID
    tx_header.DLC = len;          // Min_Data = 0 and Max_Data = 8
    tx_header.RTR = CAN_RTR_DATA; // 数据帧
    tx_header.IDE = CAN_ID_STD;   // 标准格式
    uint32_t tx_mailbox = 0;
    HAL_CAN_AddTxMessage(&hcan, &tx_header, data, &tx_mailbox);
}

/**
 * @brief 接收CAN消息
 *
 * @param recv_msg 接收消息指针
 * @param msg_count 接收消息数量
 */
void Int_CAN_receive_msg(CAN_Rec_MSG_t *recv_msg, uint8_t *msg_count)
{
    // 返回可用消息条数
    *msg_count = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0);
    for (uint8_t i = 0; i < *msg_count; i++)
    {
        // 读取一条消息
        CAN_Rec_MSG_t *rec_msg_temp = &recv_msg[i];
        memset(rec_msg_temp, 0, sizeof(CAN_Rec_MSG_t));
        HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &(rec_msg_temp->header), rec_msg_temp->data);
    }
}
