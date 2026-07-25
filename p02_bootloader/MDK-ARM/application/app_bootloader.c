#include "app_bootloader.h"

uint8_t app_boot_update_status = BOOT_NO_UPDATE;
static void App_flash_erase(uint8_t pages);
/**
 * @brief
 *
 */
void App_bootloader_check_update(void)
{
    // 读取3个字节的数据
    printf("Check update status...\n");
    printf("check update\n");
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
    HAL_Delay(3000);
}

uint8_t meta_app_buff[10] = {0};
// 程序在w25q32中保存的地址
uint32_t app_start_addr = 0;
// 需要写入到flash的程序大小
uint32_t app_size = 0;

// 一次能够写入1页的flash的缓冲区
uint8_t flash_data_buff[2049] = {0};

static void App_bootloader_check_meta_data()
{
    app_start_addr = meta_app_buff[0] | (meta_app_buff[1] << 8) | (meta_app_buff[2] << 16) | (meta_app_buff[3] << 24);
    app_size = meta_app_buff[4] | (meta_app_buff[5] << 8) | (meta_app_buff[6] << 16) | (meta_app_buff[7] << 24);
    // 块，扇，页，地址
    // 假设程序存储的地址不能在第一扇,0x00 1 000
    if (app_start_addr < 0x001000 || app_start_addr == 0xFFFFFFFF)
    {
        // 校验失败
        printf("Check failed!\n");
        return;
    }
    if (app_size < 500 || app_size > 0x78000)
    {
        // 校验失败
        printf("app_size is invalid!\n");
        return;
    }
    // 读取程序判断头两个字节
    Int_w25q32_read_data_with_32addr(app_start_addr, meta_app_buff, 8);
    // 栈顶地址
    uint32_t app_stack_ptr = meta_app_buff[0] | (meta_app_buff[1] << 8) | (meta_app_buff[2] << 16) | (meta_app_buff[3] << 24);
    // 复位
    uint32_t app_reset_handle = meta_app_buff[4] | (meta_app_buff[5] << 8) | (meta_app_buff[6] << 16) | (meta_app_buff[7] << 24);
    if (app_stack_ptr & 0xffff0000 != (STACK_ADDR))
    {
        printf("stack ptr error\n");
        return;
    }

    // 1.2 校验复位中断地址 0x0800 4xxx
    if (app_reset_handle < App_START || app_reset_handle > STACK_END)
    {
        printf("reset handle error\n");
        return;
    }
}

static void App_bootloader_write_app_flash()
{
    // 写入应用程序到flash w25q32中
    // 1.读取源数据信息，描述后续的程序
    // 前4个字节是程序的起始地址，后4个字节是程序的大小,低位在前（自定义）
    Int_w25q32_read_data(META_APP_ADDR_BLOCK, META_APP_ADDR_SECTOR,
                         META_APP_ADDR_PAGE, META_APP_ADDR_ADDR, meta_app_buff, 8);
    // 2.校验数据是否完整
    App_bootloader_check_meta_data();
    // 3.写入数据到flash
    // 擦除足够的页数
    App_flash_erase((app_size / FLASH_PAGE_SIZE) + 1);
    // 读出一页的内容
    // 剩余程序的大小
    uint32_t app_size_left = app_size;
    uint8_t data_temp = 0;
    uint32_t write_date_size = 0;
    HAL_FLASH_Unlock();
    while (app_size_left > FLASH_PAGE_SIZE)
    {
        // 已经写入到flash的数据大小
        write_date_size = app_size - app_size_left;
        // 程序剩下的大小大于1页,读出一页的内容,这里是W25Q32的地址
        Int_w25q32_read_data_with_32addr(app_start_addr + write_date_size,
                                         flash_data_buff, FLASH_PAGE_SIZE);
        // 数据大小减少
        app_size_left -= FLASH_PAGE_SIZE;
        for (int i = 0; i < FLASH_PAGE_SIZE; i += 2)
        {
            // 写入需要芯片的地址
            if (i + 1 < FLASH_PAGE_SIZE)
            {
                data_temp = flash_data_buff[i] | flash_data_buff[i + 1] << 8;
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                  App_START + write_date_size + i, data_temp);
            }
        }
    }
    // 写入最后一页
    if (app_size_left > 0)
    {
        write_date_size = app_size - app_size_left;
        // 写入剩下的程序
        Int_w25q32_read_data_with_32addr(app_start_addr + write_date_size,
                                         flash_data_buff, app_size_left);
        // 写入需要芯片的地址
        for (int i = 0; i < app_size_left; i += 2)
        {
            // 写入需要芯片的地址
            if (i + 1 < app_size_left)
            {
                data_temp = flash_data_buff[i] | flash_data_buff[i + 1] << 8;
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                  App_START + write_date_size + i, data_temp);
            }
        }
    }
    HAL_FLASH_Lock();
}

/**
 * @brief 一次写入Flash一页，擦除一页，2k一页
 *
 */
static void App_flash_erase(uint8_t pages)
{
    HAL_FLASH_Unlock();
    // 判断当前地址是否为新的一页，如果是需要擦除,擦除后全是ff，擦出前是不同的内容
    // 需要先擦除，后写入flash
    // 直接擦除足够的页大小
    FLASH_EraseInitTypeDef flash_erase_init_struct = {0};
    flash_erase_init_struct.TypeErase = FLASH_TYPEERASE_PAGES; // 页擦除
    flash_erase_init_struct.Banks = FLASH_BANK_1;              // 选择BANK1
    flash_erase_init_struct.PageAddress = App_START;      // 页地址
    flash_erase_init_struct.NbPages = pages;                   // 擦除pages页
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&flash_erase_init_struct, &page_error);
    HAL_FLASH_Lock();
}

void App_bootloader_update(void)
{
    if (app_boot_update_status == BOOT_UPDATE)
    {
        // 执行更新操作,将w25q32的数据写入到flash中
        // todo: 将W25Q32的程序写入到flash中
        printf("Need update!\n");
        App_bootloader_write_app_flash();
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
        Int_Bootloader_jump_app(RESET_START);
    }
    else
    {
        printf("No need reset!\n");
        // 不需要恢复出厂设置，跳转到正常的应用程序，0x0800 8000
        Int_Bootloader_jump_app(App_START);
    }
}
