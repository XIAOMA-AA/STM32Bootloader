#ifndef __APP_UPDATE_H__
#define __APP_UPDATE_H__

#include "usart.h"
#include "Int_can.h"
#include "crc.h"


#define BOOTLOADER_UART_REC_BUFF_LEN 32
#define APP_SEND_ID 0x000
#define APP_SEND_CMD "sss"
#define APP_SEND_CMD_LEN 3

#define APP_DATA_BUFF_LEN 16384 // 16KB

// 程序状态机
typedef enum {
    APP_UPDATE_IDLE = 0,
    APP_UPDATE_SEND_CMD,
    APP_UPDATE_RECV_DATA,
    APP_UPDATE_CHECK_DATA,
    APP_UPDATE_CHANGE_BOOT_MODE,
} app_update_state_t;

/**
 * @brief 接收串口数据 =》 收到更新标记 =》 发送can的更新指令
 * 
 */
void app_update_init(void);

/**
 * @brief 使用can发送更新指令
 * 
 */
void app_update_send_update_cmd(void);

/**
 * @brief can接收应用数据 =》 解析数据 =》 保存数据到w25q32中
 * 
 */
void app_update_receive_app_data(void);

/**
 * @brief 修改在w24c02中的更新标志位
 * 
 */
void app_update_change_boot_mode(void);

/**
 * @brief 循环调用，执行状态机逻辑
 * 
 */
void app_update_work(void);

#endif
