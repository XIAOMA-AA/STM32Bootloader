#include "app_bootloader.h"

uint8_t app_boot_update_status = BOOT_NO_UPDATE;

/**
 * @brief
 *
 */
void App_bootloader_check_update(void)
{
    // 读取3个字节的数据
    uint8_t data[3];
    Int_w24c02_read_bytes(CHECK_UPDATE_ADDR, data, 3);
    // 判断校验密钥是否正确,默认高八位在前
    uint16_t check_key = (data[1] << 8) | data[2];
    // printf("check_key = 0x%04X\n", check_key);
    // printf("data 0 = 0x%02X data 1 = 0x%02X data 2 = 0x%02X check_key = 0x%04X\n", data[0], data[1], data[2], check_key);
    if (check_key != CHECK_KEY)
    {
        // 重置密钥
        printf("Check key is incorrect!\n");
        data[0] = BOOT_NO_UPDATE;
        data[1] = CHECK_KEY >> 8;   // 高八位
        data[2] = CHECK_KEY & 0xFF; // 低八位
        Int_w24c02_write_bytes(CHECK_UPDATE_ADDR, data, 3);
        HAL_Delay(5);
        return;
    }
    else
    {
        // 校验密钥正确,判断是否需要更新
        printf("password is correct!\n");
        printf("中文测试\n");
        app_boot_update_status = data[0];
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == key1_Pin)
    {
        // 处理key1按键中断
        app_boot_update_status = BOOT_RESET;
        // printf("Key1 pressed! Set update status to BOOT_RESET.\n");
    }
}

void App_bootloader_check_default(void)
{
    HAL_Delay(5000);
}

void App_bootloader_update(void)
{
    if (app_boot_update_status == BOOT_UPDATE)
    {
        // 执行更新操作,将w25q32的数据写入到flash中
        // todo: 将W25Q32的程序写入到flash中
        printf("Need update!\n");
    }
    else if (app_boot_update_status == BOOT_NO_UPDATE)
    {
        // 不需要更新
        printf("No need update!\n");
    }
    else if (app_boot_update_status == BOOT_RESET)
    {
        // 恢复出厂设置
        printf("Reset!\n");
    }
}

/**
 * @brief 跳转到应用程序
 *
 */
void App_bootloader_jump_app(void)
{
    // 跳转到应用程序的入口地址,不管更新与否，都需要跳到a程序
    if (app_boot_update_status == BOOT_RESET)
    {
        // 跳转到出厂设置默认程序，0x0800 4000
        printf("Reset to default!\n");
    }
    else
    {
        printf("No need reset!\n");
        // 不需要恢复出厂设置，跳转到正常的应用程序，0x0800 8000
    }
    printf("Jump to app!\n");
    Int_Bootloader_jump_app();
}
