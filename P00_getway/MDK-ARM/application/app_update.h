#ifndef __APP_UPDATE_H__
#define __APP_UPDATE_H__

#include "fdcan.h"
#include "usart.h"
#include "Int_can.h"
#include "Int_bootloader.h"
#include "crc.h"

#define APP_UPDATE_CMD "sss"
#define APP_UPDATE_CMD_ID 0x00

typedef enum{
    APP_UPDATE_WAIT_CMD = 0,
    APP_UPDATE_SEND_APP,
}app_update_state_t;

/**
 * @brief 初始化上位机更新程序
 * 
 */
void App_Update_Init(void);

/**
 * @brief 等待开发板更新请求
 * 
 */
void App_Update_wait_cmd(void);

/**
 * @brief 发送更新程序给开发板
 * 
 */
void App_Update_send_app(void);

/**
 * @brief 循环程序
 * 
 */
void App_Update_work(void);

#endif
