#ifndef __APP_BOOTLOADER_H__
#define __APP_BOOTLOADER_H__

#include "Int_w24c02.h"
#include "Int_bootloader.h"
#include "Int_w25q32.h"

// 添加校验密钥
#define CHECK_KEY_ADDR 0x11 // 存储校验密钥的地址
#define CHECK_KEY 0x5A6B    // 校验密钥

// 存储更新状态的位置
#define CHECK_UPDATE_ADDR 0x10 // 存储是否需要更新的标志位地址

#define BOOT_UPDATE 0X01    // 需要更新标志位
#define BOOT_NO_UPDATE 0X00 // 不需要更新标志位

// 恢复出厂设置
#define BOOT_RESET 0x03

#define META_APP_ADDR_BLOCK 0x00000000  // 应用程序的起始地址
#define META_APP_ADDR_SECTOR 0x00000000 // 应用程序的起始地址
#define META_APP_ADDR_PAGE 0x00000000   // 应用程序的起始地址
#define META_APP_ADDR_ADDR 0x00000000   // 应用程序的起始地址

#define APP_START_ADDR_MIN 0x001000 // 应用程序的起始地址不能在第一扇,0x00 1 000
#define APP_SIZE_MIN 500            // 应用程序的最小大小为500字节
#define APP_SIZE_MAX 0x78000        // 应用程序的大小不能超过512k-16k

// #define FLASH_PAGE_SIZE 2048 // 芯片Flash一页的大小为2k

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
