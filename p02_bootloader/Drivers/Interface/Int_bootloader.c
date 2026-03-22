#include "Int_bootloader.h"

/**
 * @brief 使用串口发送会丢数据。> 解决办法
 *  1. 加大缓冲区，加内存
 *  2. 使用性能更好的通信协议
 *  3. 可以使用usart的dma接收,作用不大
 *  4. 降低波特率
 */

#define Bootloader_Receive_Buffer_Size 512
uint8_t Bootloader_Receive_Buffer[Bootloader_Receive_Buffer_Size];
uint16_t uart_rec_len = 0;
uint32_t uart_rec_full_len = 0;
uint8_t is_bootloader = 0;
// 写入位置的偏移量
uint16_t bootloarder_offset = 0;

// 记录当前一次接收记录的时间
uint32_t last_rec_time = 0;

// 末尾可能出现的单独字节
uint8_t last_byte_flag = 0;
uint8_t last_byte = 0;

static void Int_flash_erase(void)
{

    // 判断当前地址是否为新的一页，如果是需要擦除,擦除后全是ff，擦出前是不同的内容
    uint8_t is_erase = 0;
    uint32_t page_addr = 0;
    for (size_t i = 0; i < uart_rec_len; i++)
    {
        // 读取当前地址的值
        uint8_t data = *(volatile uint8_t *)(App_Address + i + bootloarder_offset);
        if (data != 0xff)
        {
            // printf("erase %d %d %c", i, bootloarder_offset, data);
            is_erase = 1;
            page_addr = (App_Address + bootloarder_offset + i) - (App_Address + bootloarder_offset + i) % FLASH_PAGE_SIZE;
            break;
        }
    }

    if (is_erase)
    {
        // 需要先擦除，后写入flash
        FLASH_EraseInitTypeDef flash_erase_init_struct = {0};
        flash_erase_init_struct.TypeErase = FLASH_TYPEERASE_PAGES; // 页擦除
        flash_erase_init_struct.Banks = FLASH_BANK_1;              // 选择BANK1
        flash_erase_init_struct.PageAddress = page_addr;           // 页地址
        flash_erase_init_struct.NbPages = 1;                       // 擦除1页
        uint32_t page_error = 0;
        HAL_FLASHEx_Erase(&flash_erase_init_struct, &page_error);
        is_erase = 0;
    }
}

static void Int_flash_write_with_last(void)
{
    // 上次遗留有一个字节，这次作为第一个字节写入
    for (uint16_t i = 0; i < uart_rec_len; i += 2)
    {
        uint16_t buff;
        // 写入地址 = 应用程序地址 + 偏移量 + 当前字节索引
        uint32_t flash_addr = App_Address + i + bootloarder_offset;
        if (i == 0)
        {
            // 拼接上次遗留的字节和当前第一个字节
            buff = last_byte | (Bootloader_Receive_Buffer[i] << 8);
        }
        else
        {
            // i 走到rec len - 1,如果rec len=9，i走到8，加上次遗留，发送10字节，不遗留，但是多一个字节长度
            buff = Bootloader_Receive_Buffer[i - 1] | (Bootloader_Receive_Buffer[i] << 8);
        }
        // 写入flash
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr, buff);
    }
}

static void Int_flash_write_no_last(void)
{
    // 正好可以写入
    // 写入flash
    for (size_t i = 0; i < uart_rec_len; i += 2)
    {
        // 拼接数据，原始bin文件是大端模式，但是stm32写入flash是小端模式，需要交换字节顺序
        // 大端序：高位在低地址，低位在高地址
        // 小端序：低位在低地址，高位在高地址
        uint16_t buff;
        // 写入地址 = 应用程序地址 + 偏移量 + 当前字节索引
        uint32_t flash_addr = App_Address + i + bootloarder_offset;
        if (i + 1 < uart_rec_len)
        {
            buff = Bootloader_Receive_Buffer[i] | (Bootloader_Receive_Buffer[i + 1] << 8);
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr, buff);
        }
    }
}

static void Int_flash_write_halfword(void)
{
    // 判断当前写入的是否为偶数字节
    /*
        rec_len 有两种情况 119 120如果是119 + 标志位1 那就是120，如果是120 + 标志位0 那就是121
    */
    // 其实就是rec len是奇数还是偶数字节的问题
    if ((uart_rec_len + last_byte_flag) % 2 == 0)
    {
        // 如果rec是奇数字节，就拼接上次的
        if (last_byte_flag)
        {
            Int_flash_write_with_last();
            bootloarder_offset += uart_rec_len + 1;
            last_byte_flag = 0;
        }
        // 否则如果rec len是偶数字节，直接发送
        else
        {
            Int_flash_write_no_last();
            bootloarder_offset += uart_rec_len;
        }
    }
    // 这次写入还有保留一个字节,奇数字节，剩一个下次写入
    // 如果rec len是偶数，last_byte_flag = 1
    else
    {
        // 如果rec len是偶数，并且上次剩了一个字节，那么这次就写入奇数字节，会剩余一个
        if (last_byte_flag)
        {
            // 上次有遗留字节，这次数量发送的是偶数，一共是奇数字节，会剩余一个字节,
            //  实际还是写入偶数字节uart_rec_len的长度，剩余最后一个字节下次写入
            // 拼接上次遗留的字节和当前字节
            // for (uint16_t i = 0; i < uart_rec_len; i += 2)
            // {
            //     uint16_t buff;
            //     // 写入地址 = 应用程序地址 + 偏移量 + 当前字节索引
            //     uint32_t flash_addr = App_Address + i + bootloarder_offset;
            //     if (i == 0)
            //     {
            //         // 拼接上次遗留的字节和当前字节
            //         buff = last_byte | (Bootloader_Receive_Buffer[i] << 8);
            //     }
            //     else
            //     {
            //         // i 走到8,一共发出去10个字节，最后一个保留
            //         buff = Bootloader_Receive_Buffer[i - 1] | (Bootloader_Receive_Buffer[i] << 8);
            //     }
            //     // 写入flash
            //     HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr, buff);
            // }
            Int_flash_write_with_last();
            // 修改上次字节
            last_byte = Bootloader_Receive_Buffer[uart_rec_len - 1];
            bootloarder_offset += uart_rec_len;
        }
        // 如果rec len是9奇数，并且上次没有剩余，那么还是走这个分支，剩余一个字节
        else
        {
            // 上次没有遗留字节，这次会留下一个字节
            // 保留当前字节，下次会作为第一个字节写入
            // for (size_t i = 0; i < uart_rec_len; i += 2)
            // {
            //     // 拼接数据，原始bin文件是大端模式，但是stm32是小端模式，需要交换字节顺序
            //     uint16_t buff;
            //     // 写入地址 = 应用程序地址 + 偏移量 + 当前字节索引
            //     uint32_t flash_addr = App_Address + i + bootloarder_offset;
            //     buff = Bootloader_Receive_Buffer[i] | (Bootloader_Receive_Buffer[i + 1] << 8);
            //     HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flash_addr, buff);
            // }
            Int_flash_write_no_last();
            // 少写入一个
            bootloarder_offset += uart_rec_len - 1;
            last_byte = Bootloader_Receive_Buffer[uart_rec_len - 1];
        }
        last_byte_flag = 1;
    }
}

/**
 * @brief 接收中断回调函数
 *  串口协议稳定性差，发送长文件容易丢失字节
 *  修改波特率可以提升稳定性 》》高波特率性能比较高
 *  如果在中断中调用printf串口打印，会非常占用资源，建议在主循环中打印
 * @param huart
 * @param size
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart->Instance == USART1)
    {
        // 记录当前时间
        last_rec_time = HAL_GetTick();
        // 每次接收的字节数
        uart_rec_len = size;
        // 累计接收字节数
        uart_rec_full_len += uart_rec_len;
        // 解锁flash
        HAL_FLASH_Unlock();
        // 先判断是否需要擦除
        Int_flash_erase();
        // 写入字节
        Int_flash_write_halfword();
        // 加锁
        HAL_FLASH_Lock();

        // 清空接收缓存
        memset(Bootloader_Receive_Buffer, 0, Bootloader_Receive_Buffer_Size);
        // 清空掉初始化串口使用的缓存
        __HAL_UART_CLEAR_OREFLAG(&huart1);
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        // 启动下一次接收
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)Bootloader_Receive_Buffer, Bootloader_Receive_Buffer_Size);
    }
}

/**
 * @brief 初始化串口接收
 *
 */
void Int_Bootloader_receive_app(void)
{
    // 清空掉初始化串口使用的缓存
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);

    // 启动空闲中断接收
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)Bootloader_Receive_Buffer, Bootloader_Receive_Buffer_Size);
}

/**
 * @brief 跳转应用程序
 *  0:成功 1:失败
 */
uint8_t Int_Bootloader_jump_app(void)
{
    typedef void (*pFunc)(void);

    // 1 校验
    // 获取栈顶指针的值
    uint32_t app_stack_ptr = *(volatile uint32_t *)(App_Address);
    // 复位中断地址 = 应用程序地址 + 4
    uint32_t app_reset_handle = *(volatile uint32_t *)(App_Address + 4);

    // 1.1 校验栈顶地址
    if (app_stack_ptr & 0xffff0000 != (STACK_ADDR))
    {
        printf("stack ptr error\n");
        return 1;
    }

    // 1.2 校验复位中断地址 0x0800 4xxx
    if (app_reset_handle < App_Address || app_reset_handle > STACK_END)
    {
        printf("reset handle error\n");
        return 1;
    }

    // 2 注销bootloader程序

    // 2.1 手动注销内核
    NVIC_DisableIRQ(EXTI9_5_IRQn);
    NVIC_DisableIRQ(USART1_IRQn);

    // 2.2 关闭systick
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    SysTick->LOAD = 0;

    // 2.3 注销hal库,只会注销外设，不会注销内核
    HAL_DeInit();

    // 2.4 关闭中断
    __disable_irq();

    // 2.5 设置堆栈指针
    __set_MSP(app_stack_ptr);

    // 2.6 重定向中断向量表 -> 应用程序复位中断地址
    SCB->VTOR = App_Address;

    // 2.7 跳转a程序复位中断。把复位中断地址转换为函数指针，调用函数
    pFunc jum_to_app = (pFunc)app_reset_handle;
    jum_to_app();
    
    // 执行不到
    printf("jump app sucess\n");
    return 0;
}

/**
 * @brief 擦除flash页,外部可调用提前擦除flash
 *
 * @param page_addr 页地址
 * @param pages 页数量
 */
void Int_Bootloader_erase_flash(uint32_t page_addr, uint16_t pages)
{
    // 解锁flash
    HAL_FLASH_Unlock();
    // 擦除flash页
    // 需要先擦除，后写入flash
    FLASH_EraseInitTypeDef flash_erase_init_struct = {0};
    flash_erase_init_struct.TypeErase = FLASH_TYPEERASE_PAGES; // 页擦除
    flash_erase_init_struct.Banks = FLASH_BANK_1;              // 选择BANK1
    flash_erase_init_struct.PageAddress = page_addr;           // 页地址
    flash_erase_init_struct.NbPages = pages;                   // 擦除pages页数
    uint32_t page_error = 0;
    // 擦除flash页
    HAL_FLASHEx_Erase(&flash_erase_init_struct, &page_error);
    // 加锁
    HAL_FLASH_Lock();
}
