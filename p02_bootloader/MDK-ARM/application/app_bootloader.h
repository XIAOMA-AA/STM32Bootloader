#ifndef __APP_BOOTLOADER_H__
#define __APP_BOOTLOADER_H__

#include "Int_w24c02.h"
#include "Int_bootloader.h"

// 添加校验密钥
#define CHECK_KEY_ADDR 0x11 // 存储校验密钥的地址
#define CHECK_KEY 0x5A6B    // 校验密钥

// 存储更新状态的位置
#define CHECK_UPDATE_ADDR 0x10 // 存储是否需要更新的标志位地址

#define BOOT_UPDATE 0X01       // 需要更新标志位
#define BOOT_NO_UPDATE 0X00    // 不需要更新标志位

// 恢复出厂设置
#define BOOT_RESET 0x03

// 判断当前是否需要更新
/**
 * @brief
 *
 */
void App_bootloader_check_update(void);

/**
 * @brief 检查是否默认程序
 * 
 */
void App_bootloader_check_default(void);

/**
 * @brief 执行更新操作
 * 
 */
void App_bootloader_update(void);

/**
 * @brief 跳转到应用程序
 * 
 */
void App_bootloader_jump_app(void);

#endif // __APP_BOOTLOADER_H__
