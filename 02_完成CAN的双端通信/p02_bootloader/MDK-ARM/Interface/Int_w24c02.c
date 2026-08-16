#include "Int_w24c02.h"

/**
 * @brief 读取一个字节
 *
 * @param byte_addr 字节地址
 * @return uint8_t 字节值
 */
uint8_t Int_w24c02_read_byte(uint8_t byte_addr)
{
    // 流程：启动信号，写从设备地址，字节地址，启动信号，读从设备地址，读取数据，nack停止
    // 读取一个字节
    uint8_t data;
    HAL_I2C_Mem_Read(&hi2c2, W24C02_ADDR_READ, byte_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
    return data;
}

/**
 * @brief 写入一个字节
 *
 * @param byte_addr 字节地址
 * @param data 字节值
 */
void Int_w24c02_write_byte(uint8_t byte_addr, uint8_t data)
{
    // 写入一个字节
    HAL_I2C_Mem_Write(&hi2c2, W24C02_ADDR, byte_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
}

/**
 * @brief 读取多个字节
 *
 * @param byte_addr 字节地址
 * @param data 数据缓冲区
 * @param len 要读取的字节数
 */

void Int_w24c02_read_bytes(uint8_t byte_addr, uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Read(&hi2c2, W24C02_ADDR_READ, byte_addr, I2C_MEMADD_SIZE_8BIT, data, len, 1000);
}

/**
 * @brief 写入多个字节
 *
 * @param byte_addr 字节地址
 * @param data 数据缓冲区
 * @param len 要写入的字节数
 */
void Int_w24c02_write_bytes(uint8_t byte_addr, uint8_t *data, uint16_t len)
{
    // 一页最多写入16个字节，具体能写入多少取决于起始地址，最多从0x00 -> 0x0f

    // 一页写满后，自动向下写入
    // 1. 循环单字节写入，代码简单，效率低
    // 2. 软件判断具体写入哪几页，一页写入一次，代码复杂，效率高

    // 地址不能超过EEPROM的地址 255
    if (byte_addr + len > 255)
    {
        printf("write data 超出EEPROM地址范围\n");
        return;
    }

    // 判断当前页的剩余空间
    // uint8_t start_page_addr = byte_addr % 16;   // 当前页已经使用的字节数
    uint8_t page_remain = 16 - byte_addr % 16; // 当前页剩余可用的字节数
    if (len < page_remain)
    {
        // 可以一次写完
        HAL_I2C_Mem_Write(&hi2c2, W24C02_ADDR, byte_addr, I2C_MEMADD_SIZE_8BIT, data, len, 1000);
    }
    else
    {
        // 下次写入的起始地址
        uint8_t start_page_addr = byte_addr;
        // 写入的页数
        uint8_t page_count = 0;
        // 分批写入
        while (len > page_remain)
        {
            // 将当前页剩余的空间写满
            //  data + page_count * 16 要写入的数据位置
            // 感觉逻辑有问题，如果第一次写入的数据长度小于16，那么会漏写数据
            HAL_I2C_Mem_Write(&hi2c2, W24C02_ADDR, start_page_addr, I2C_MEMADD_SIZE_8BIT, data + page_count * 16, page_remain, 1000);

            // 下一页的起始地址= byte_addr + page_remain
            len -= page_remain;             // 数据指针移动
            start_page_addr += page_remain; // 下一页的起始地址
            page_remain = 16;               // 后续页都是满的
            page_count++;                   // 下一页的位置
            HAL_Delay(5);                   // 写入一页后需要等待5ms以上才能继续写入，否则可能写入失败
        }
        // 最后一次写入剩余的数据
        if (len != 0)
        {
            HAL_I2C_Mem_Write(&hi2c2, W24C02_ADDR, start_page_addr, I2C_MEMADD_SIZE_8BIT, data + page_count * 16, len, 1000);
        }
    }
}
