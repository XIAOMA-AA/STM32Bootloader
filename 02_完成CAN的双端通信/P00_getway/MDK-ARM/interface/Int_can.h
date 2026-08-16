#ifndef __INT_CAN_H__
#define __INT_CAN_H__

#include "fdcan.h"
#include "stdio.h"

typedef struct
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t data[8];
}can_recv_msg_t;

void Int_fdcan_init(void);

void Int_fdcan_send(uint16_t id, uint8_t *data, uint8_t len);

void Int_fdcan_recv(can_recv_msg_t *msg, uint32_t *count);

#endif
