#include "app_update.h"

// 标记上位机当前状态
app_update_state_t g_app_update_state = APP_UPDATE_WAIT_CMD;

// 用于接收开发板发送的请求
CAN_Rec_MSG_t msg[3] = {0};
uint8_t msg_count = 0;

// 记录程序的总长度
extern uint16_t uart_rec_full_len;
// 已经发送的数据长度
uint16_t update_data_len = 0;
// can发送数据的缓冲区
uint8_t update_buff[8] = {0};

/**
 * @brief 初始化上位机更新程序
 *
 */
void App_Update_Init(void)
{
    printf("App_Update_Init\n");
    g_app_update_state = APP_UPDATE_WAIT_CMD;
    Int_CAN_init();
    printf("App_Update_wait_cmd\n");

    // 如果已经烧录好更新的程序，需要手动填写一下更新程序的长度
    if(uart_rec_full_len == 0)
    {
        uart_rec_full_len = 3956;
    }
}

/**
 * @brief 等待开发板更新请求
 *
 */
void App_Update_wait_cmd(void)
{
    Int_CAN_receive_msg(msg, &msg_count);
    for (uint8_t i = 0; i < msg_count; i++)
    {
        // 只接受ID为0的数据
        if (msg[i].header.StdId == 0)
        {
            // 判断是否为更新消息 "sss"
            if (strcmp((char *)msg[i].data, APP_UPDATE_CMD) == 0)
            {
                printf("App_Update_wait_cmd: %s\n", msg[i].data);
                g_app_update_state = APP_UPDATE_SEND_APP;
                /* 接收到更新请求后，延时100ms，确保开发板有足够的时间准备 */
                HAL_Delay(100);
            }
        }
    }
}

static uint32_t app_crc_cal(uint32_t flash_addr, uint16_t len)
{
    /* 将flash_addr转换为uint32_t指针 */
    uint32_t *p_data = (uint32_t *)flash_addr;
    uint32_t word_count = (len + 3) / 4;

    // 复位CRC寄存器
    __HAL_CRC_DR_RESET(&hcrc);

    uint32_t crc_value = HAL_CRC_Calculate(&hcrc, p_data, word_count);
    return crc_value;
}

/**
 * @brief 发送更新程序给开发板
 *
 */
void App_Update_send_app(void)
{
    printf("App_Update_send_app\n");
    // 发送数据
    if (update_data_len < uart_rec_full_len)
    {
        // 要发送的字节数
        uint8_t send_len = 0;
        // 判断剩下的自己够不够8个字节
        if (uart_rec_full_len - update_data_len >= 8)
        {
            send_len = 8;
        }
        else
        {
            send_len = uart_rec_full_len - update_data_len;
        }
        // 发送剩余数据
        for (uint8_t i = 0; i < send_len; i++)
        {
            update_buff[i] = *(volatile uint8_t *)(App_Address + update_data_len + i);
        }
        update_data_len += send_len;
        /* 发送字节，200kbit/s，一条消息100bit，大概500us */
        Int_CAN_send(APP_UPDATE_CMD_ID, update_buff, send_len);

        // 如果下游接收速度不够,上游就需要添加延时，确保数据能够及时接收
        // HAL_Delay(5);

        // 每次发送足够数量的字节 256个字节，延时等待一下
        if (update_data_len % 256 == 0)
        {
            HAL_Delay(100);
        }
    }
    else
    {
        // 发送完成
        printf("App_Update_send_app: send done\n");
        update_data_len = 0;
        g_app_update_state = APP_UPDATE_WAIT_CMD;

        // 发送CRC校验值
        HAL_Delay(200);
        uint32_t crc_value = app_crc_cal(App_Address, uart_rec_full_len);
        Int_CAN_send(APP_UPDATE_CMD_ID, (uint8_t *)&crc_value, 4);
    }
}

void App_Update_work(void)
{
    switch (g_app_update_state)
    {
    case APP_UPDATE_WAIT_CMD:
        App_Update_wait_cmd();
        break;
    case APP_UPDATE_SEND_APP:
        App_Update_send_app();
        break;
    default:
        break;
    }
}
