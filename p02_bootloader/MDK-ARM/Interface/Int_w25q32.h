#ifndef __INT_W25Q32_H__
#define __INT_W25Q32_H__

#include "spi.h"
#include "usart.h"

#define W25Q32_READ_ID 0x9F       // 读取ID指令
#define W25Q32_READ_STATUS 0x05   // 读取状态寄存器指令
#define W25Q32_READ_DATA 0x03     // 读取数据指令
#define W25Q32_WRITE_DATA 0x02    // 写入数据指令
#define W25Q32_SECTOR_ERASE 0x20  // 扇区擦除指令
#define W25Q32_WRITE_ENABLE 0x06  // 写使能指令
#define W25Q32_WRITE_DISABLE 0x04 // 写禁止指令

/**
 * @brief 拉低片选
 *
 */
void Int_w25q32_start(void);

/**
 * @brief 拉高片选
 *
 */
void Int_w25q32_stop(void);

/**
 * @brief 写入一个字节
 *
 * @param byte 要写入的字节
 */
void Int_w25q32_write_byte(uint8_t byte);

/**
 * @brief 读取一个字节
 *
 * @return uint8_t 读取到的字节
 */
uint8_t Int_w25q32_read_byte(void);

/**
 * @brief 读取w25q32的ID
 *
 * @param mf_id 主工厂ID
 * @param dev_id 设备ID
 */
void Int_w25q32_read_id(uint8_t *mf_id, uint16_t *dev_id);

/**
 * @brief 读取多个字节
 *
 * @param block_addr 块地址
 * @param sector_addr 扇区地址
 * @param page_addr 页地址
 * @param addr 读取的地址
 * @param data 数据缓冲区
 * @param len 要读取的字节数
 */
void Int_w25q32_read_data(uint8_t block_addr, uint8_t sector_addr, uint8_t page_addr, uint8_t addr, uint8_t *data, uint16_t len);

/**
 * @brief  写入多个字节
 *
 * @param block_addr 块地址
 * @param sector_addr 扇区地址
 * @param page_addr 页地址
 * @param addr 写入的地址
 * @param data 数据缓冲区
 * @param len 要写入的字节数
 */
void Int_w25q32_write_data(uint8_t block_addr, uint8_t sector_addr, uint8_t page_addr, uint8_t addr, uint8_t *data, uint16_t len);

/**
 * @brief 擦除扇区
 *
 * @param block_addr 块地址
 * @param sector_addr 扇区地址
 */
void Int_w25q32_erase_sector(uint8_t block_addr, uint8_t sector_addr);

static void Int_w25q32_write_enable(void);

static void Int_w25q32_write_disable(void);

#endif /* __INT_W25Q32_H__ */
