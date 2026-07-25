#ifndef __INT_W24C02_H__
#define __INT_W24C02_H__


#include "i2c.h"
#include "usart.h"
/***********************W24C02的接口函数***************************/
#define W24C02_ADDR 0xA0 // W24C02的I2C地址
#define W24C02_ADDR_READ 0xA1 // W24C02的I2C地址，读取

#define W24C02_ADDR_SIZE 8 //
#define W24C02_PAGE_SIZE 16 // W24C02的页大小

/**
 * @brief 读取一个字节
 * 
 * @param byte_addr 字节地址
 * @return uint8_t 字节值
 */
uint8_t Int_w24c02_read_byte(uint8_t byte_addr);

/**
 * @brief 写入一个字节
 * 
 * @param byte_addr 字节地址
 * @param data 字节值
 */
void Int_w24c02_write_byte(uint8_t byte_addr, uint8_t data);
    
/**
 * @brief 读取多个字节
 * 
 * @param byte_addr 字节地址
 * @param data 数据缓冲区
 * @param len 要读取的字节数
 */

void Int_w24c02_read_bytes(uint8_t byte_addr, uint8_t *data, uint16_t len);

/**
 * @brief 写入多个字节
 * 
 * @param byte_addr 字节地址
 * @param data 数据缓冲区
 * @param len 要写入的字节数
 */
void Int_w24c02_write_bytes(uint8_t byte_addr, uint8_t *data, uint16_t len);

#endif
