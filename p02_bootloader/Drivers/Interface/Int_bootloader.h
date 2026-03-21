#ifndef __INT_BOOTLOADER_H__
#define __INT_BOOTLOADER_H__



#include "usart.h"
#include "string.h"
// 最先进B区，然后校验完成跳转A区，所以A区从0x0800 4000开始，大小0x7c000
// 应用程序A区起始地址，假设B区的大小为16k 0x4000，A区 = 512k-16k
#define App_Address 0x08004000 // 应用程序地址

#define STACK_ADDR 0x20000000 // 栈顶地址
#define STACK_END 0x08080000 // 栈底地址

/**
 * @brief 
 * 
 */
void Int_Bootloader(void);

/**
 * @brief 跳转应用程序
 * 
 */
void Int_Bootloader_jump_app(void);

#endif
