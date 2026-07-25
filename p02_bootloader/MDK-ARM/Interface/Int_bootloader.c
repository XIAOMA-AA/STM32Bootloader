#include "Int_bootloader.h"

/**
 * @brief 使用串口发送会丢数据。> 解决办法
 *  1. 加大缓冲区，加内存
 *  2. 使用性能更好的通信协议
 *  3. 可以使用usart的dma接收,作用不大
 *  4. 降低波特率
 */


/**
 * @brief 跳转应用程序
 *  0:成功 1:失败
 */
uint8_t Int_Bootloader_jump_app(uint32_t app_start_addr)
{
    typedef void (*pFunc)(void);

    // 1 校验
    // 获取栈顶指针的值
    uint32_t app_stack_ptr = *(volatile uint32_t *)(app_start_addr);
    // 复位中断地址 = 应用程序地址 + 4
    uint32_t app_reset_handle = *(volatile uint32_t *)(app_start_addr + 4);

    // 1.1 校验栈顶地址是否在栈地址范围内
    if (app_stack_ptr & 0xffff0000 != (STACK_ADDR))
    {
        printf("stack ptr error\n");
        return 1;
    }

    // 1.2 校验复位中断地址 0x0800 4xxx
    if (app_reset_handle < app_start_addr || app_reset_handle > STACK_END)
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
    SCB->VTOR = app_start_addr;

    // 2.7 跳转a程序复位中断。把复位中断地址转换为函数指针，调用函数
    pFunc jum_to_app = (pFunc)app_reset_handle;
    jum_to_app();
    
    // 执行不到
    printf("jump app sucess\n");
    return 0;
}
