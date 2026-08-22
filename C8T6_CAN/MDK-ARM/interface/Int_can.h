#ifndef __INT_CAN_H__
#define __INT_CAN_H__

#include "can.h"
#include "string.h"
#include "stdio.h"

typedef struct
{
    CAN_RxHeaderTypeDef  header;
    uint8_t data[8];
} CAN_Rec_MSG_t;

/**
 * @brief 配置白名单过滤器，手动开启工作模式，默认是休眠模式
 *
 */
void Int_CAN_init(void);

/**
 * @brief 发送CAN消息
 *
 * @param id CAN ID，标准ID是11位，扩展ID是29位
 * @param data 数据指针
 * @param len 数据长度
 */
void Int_CAN_send(uint16_t id, uint8_t *data, uint8_t len);

/**
 * @brief 接收CAN消息
 *
 * @param recv_msg 接收消息数组，最多一次可以获得3条消息
 * @param msg_count 接收消息数量
 */
void Int_CAN_receive_msg(CAN_Rec_MSG_t *recv_msg, uint8_t *msg_count);

#endif
