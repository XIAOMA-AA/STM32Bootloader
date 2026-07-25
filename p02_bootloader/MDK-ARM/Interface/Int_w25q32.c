#include "Int_w25q32.h"

/**
 * @brief 拉低片选
 *
 */
void Int_w25q32_start(void)
{
    HAL_GPIO_WritePin(W25Q32_CS_GPIO_Port, W25Q32_CS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 拉高片选
 *
 */
void Int_w25q32_stop(void)
{
    HAL_GPIO_WritePin(W25Q32_CS_GPIO_Port, W25Q32_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief 写入一个字节
 *
 * @param byte 要写入的字节
 */
void Int_w25q32_write_byte(uint8_t byte)
{
    HAL_SPI_Transmit(&hspi1, &byte, 1, 0xFFFF);
}

/**
 * @brief 读取一个字节
 *
 * @return uint8_t 读取到的字节
 */
uint8_t Int_w25q32_read_byte(void)
{
    uint8_t byte = 0;
    HAL_SPI_Receive(&hspi1, &byte, 1, 0xFFFF);
    return byte;
}

/**
 * @brief 读取w25q32的ID
 *
 * @param mf_id 主工厂ID
 * @param dev_id 设备ID
 */
void Int_w25q32_read_id(uint8_t *mf_id, uint16_t *dev_id)
{
    Int_w25q32_start();

    Int_w25q32_write_byte(W25Q32_READ_ID);

    *mf_id = Int_w25q32_read_byte();
    uint8_t high = Int_w25q32_read_byte();
    uint8_t low = Int_w25q32_read_byte();
    *dev_id = (high << 8) | low;

    Int_w25q32_stop();
}

/**
 * @brief 等待w25q32空闲，静态方法
 *
 */
static void Int_w25q32_wait_busy(void)
{
    Int_w25q32_start();
    Int_w25q32_write_byte(W25Q32_READ_STATUS); // 读取状态寄存器指令
    uint8_t status;
    do
    {
        // status寄存器的第0位是忙标志位，1表示忙，0表示空闲
        status = Int_w25q32_read_byte();
    } while (status & 0x01); // 检查忙标志位
    Int_w25q32_stop();
}

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
void Int_w25q32_read_data(uint8_t block_addr, uint8_t sector_addr, uint8_t page_addr, uint8_t addr, uint8_t *data, uint16_t len)
{
    Int_w25q32_write_enable();

    // 拉低片选
    Int_w25q32_start();

    // 发送读取指令
    Int_w25q32_write_byte(W25Q32_READ_DATA);
    // sector 与 page 共同构成 8-16位，分别是sector的低四位与page的低四位
    uint32_t read_addr = block_addr << 16 | sector_addr << 12 | page_addr << 8 | addr;
    Int_w25q32_write_byte((read_addr >> 16) & 0xFF); // 发送高8位地址
    Int_w25q32_write_byte((read_addr >> 8) & 0xFF);  // 发送中8位地址
    Int_w25q32_write_byte(read_addr & 0xFF);         // 发送低8位地址
    // 读取数据
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = Int_w25q32_read_byte();
    }
    // 拉高片选
    Int_w25q32_stop();
}

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
void Int_w25q32_write_data(uint8_t block_addr, uint8_t sector_addr, uint8_t page_addr, uint8_t addr, uint8_t *data, uint16_t len)
{
    Int_w25q32_write_enable();
    // 拉低片选
    Int_w25q32_start();

    uint32_t write_addr = block_addr << 16 | sector_addr << 12 | page_addr << 8 | addr;
    // 发送写入指令
    Int_w25q32_write_byte(W25Q32_WRITE_DATA);
    Int_w25q32_write_byte((write_addr >> 16) & 0xFF); // 发送高8位地址
    Int_w25q32_write_byte((write_addr >> 8) & 0xFF);  // 发送中8位地址
    Int_w25q32_write_byte(write_addr & 0xFF);         // 发送低8位地址

    // 写入数据
    for (uint16_t i = 0; i < len; i++)
    {
        Int_w25q32_write_byte(data[i]);
    }
    // 拉高片选
    Int_w25q32_stop();
}

/**
 * @brief 擦除扇区
 *
 * @param block_addr 块地址
 * @param sector_addr 扇区地址
 */
void Int_w25q32_erase_sector(uint8_t block_addr, uint8_t sector_addr)
{
    Int_w25q32_write_enable();
    // 拉低片选
    Int_w25q32_start();

    uint32_t erase_addr = block_addr << 16 | sector_addr << 12;
    // 发送扇区擦除指令
    Int_w25q32_write_byte(W25Q32_SECTOR_ERASE);
    Int_w25q32_write_byte((erase_addr >> 16) & 0xFF); // 发送高8位地址
    Int_w25q32_write_byte((erase_addr >> 8) & 0xFF);  // 发送中8位地址
    Int_w25q32_write_byte(0);                         // 发送低8位地址

    // 拉高片选
    Int_w25q32_stop();
}

static void Int_w25q32_write_enable(void)
{
    // 等待忙
    Int_w25q32_wait_busy();
    // 拉低片选
    Int_w25q32_start();

    // 发送写使能指令
    Int_w25q32_write_byte(W25Q32_WRITE_ENABLE);

    // 拉高片选
    Int_w25q32_stop();
}

static void Int_w25q32_write_disable(void)
{
    // 等待忙
    Int_w25q32_wait_busy();
    // 拉低片选
    Int_w25q32_start();

    // 发送写禁用指令
    Int_w25q32_write_byte(W25Q32_WRITE_DISABLE);

    // 拉高片选
    Int_w25q32_stop();
}
