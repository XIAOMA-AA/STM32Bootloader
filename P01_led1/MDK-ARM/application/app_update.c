#include "app_update.h"

uint8_t uart_rec_buff[BOOTLOADER_UART_REC_BUFF_LEN] = {0};
app_update_state_t app_update_state = APP_UPDATE_IDLE;

// can 接收消息的缓冲区
CAN_Rec_MSG_t can_recv_msg[3] = {0};
// 单次接收消息的条数
uint8_t can_recv_count = 0;
uint16_t can_rec_msg_len = 0;

/* 处理上下游波特率不匹配的问题 */
// 1. 声明一个可以容纳整个程序的静态缓冲区
uint8_t app_data_buff[APP_DATA_BUFF_LEN] = {0};
// 2. 硬件条件不足，定义一个小型缓冲区，缓冲区满了就写入到存储器中，落盘

// 记录当前接收的时间
uint32_t can_rec_time = 0;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    // 串口接收到数据 -》如果是 cmd
    if (huart == &huart1 && app_update_state == APP_UPDATE_IDLE)
    {
        // 校验数据是否是 cmd => 让开发版给网关发送更新指令，can发送
        if (strstr((char *)uart_rec_buff, "cmd"))
        {
            app_update_state = APP_UPDATE_SEND_CMD;
        }
        else
        {
            app_update_state = APP_UPDATE_IDLE;
        }
    }
}
/**
 * @brief 接收串口数据 =》 收到更新标记 =》 发送can的更新指令
 *
 */
void app_update_init(void)
{
    // 启动串口接收程序
    // 清空掉初始化串口使用的缓存
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);

    // 启动空闲中断接收
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)uart_rec_buff, BOOTLOADER_UART_REC_BUFF_LEN);

    // 初始化can
    Int_CAN_init();
}

/**
 * @brief 使用can发送更新指令
 *
 */
void app_update_send_update_cmd(void)
{
    // 发送更新指令
    Int_CAN_send(APP_SEND_ID, APP_SEND_CMD, APP_SEND_CMD_LEN);
    // 更新状态
    app_update_state = APP_UPDATE_RECV_DATA;
}

/**
 * @brief can接收应用数据 =》 解析数据 =》 保存数据到w25q32中
 *
 */
void app_update_receive_app_data(void)
{
    printf("app_update_receive_app_data\n");
    // 解析数据
    Int_CAN_receive_msg(can_recv_msg, &can_recv_count);
    for (uint8_t i = 0; i < can_recv_count; i++)
    {
        // 记录当前接收的时间
        can_rec_time = HAL_GetTick();

        // 保存数据到静态缓冲区
        // app_data_buff[can_rec_msg_len] = can_recv_msg[i].data;
        memcpy(app_data_buff + can_rec_msg_len, can_recv_msg[i].data, can_recv_msg[i].header.DLC);
        // 记录数据长度
        can_rec_msg_len += can_recv_msg[i].header.DLC;
    }
    can_recv_count = 0;
    // 更新状态
    app_update_state = APP_UPDATE_CHANGE_BOOT_MODE;
    if (can_rec_time != 0 && can_rec_time + 2000 < HAL_GetTick())
    {
        // 数据接收超时或者接收完成
        printf("data receive timeout, len:%d\n", can_rec_msg_len);
        app_update_state = APP_UPDATE_CHECK_DATA;
    }
}

/**
 * @brief 修改在w24c02中的更新标志位
 *
 */
void app_update_change_boot_mode(void)
{
    // 修改在w24c02中的更新标志位
}

static void app_run(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
}

static uint32_t app_crc_cal(uint8_t *data, uint16_t len)
{
    uint32_t *p_data = (uint32_t *)data;
    uint32_t word_count = (len + 3) / 4;

    // 复位CRC
    __HAL_CRC_DR_RESET(&hcrc);

    uint32_t crc_value = HAL_CRC_Calculate(&hcrc, p_data, word_count);
    return crc_value;
}

void app_update_check_data(void)
{
    printf("app_update_check_data\n");
    Int_CAN_receive_msg(can_recv_msg, &can_recv_count);
    for (uint8_t i = 0; i < can_recv_count; i++)
    {
        // 读取发送过来的crc的值，32位4字节，低位在前
        uint32_t crc_value = can_recv_msg[i].data[0] | (can_recv_msg[i].data[1] << 8) |
                             (can_recv_msg[i].data[2] << 16) | (can_recv_msg[i].data[3] << 24);
        uint32_t cur_crc_value = app_crc_cal(app_data_buff, can_rec_msg_len);
        // 校验crc是否一致
        if (crc_value == cur_crc_value)
        {
            printf("crc check pass\n");
            // 校验通过，更新状态
            app_update_state = APP_UPDATE_CHANGE_BOOT_MODE;
        }
        else
        {
            printf("crc check fail\n");
            // 校验不通过，回滚到空闲状态
            memset(app_data_buff, 0, APP_DATA_BUFF_LEN);
            can_rec_msg_len = 0;
            can_recv_count = 0;
            can_rec_time = 0;
            app_update_state = APP_UPDATE_IDLE;
        }
    }
}

/**
 * @brief 循环调用，执行状态机逻辑
 *
 */
void app_update_work(void)
{
    switch (app_update_state)
    {
    case APP_UPDATE_IDLE:
        // 正常运行的程序（流水灯），执行更新的时候停止流水灯
        app_run();
        break;
    case APP_UPDATE_SEND_CMD:
        // 发送更新指令
        printf("send cmd\r\n");
        app_update_send_update_cmd();
        /* 接收消息前清空缓冲区 */
        memset(app_data_buff, 0, APP_DATA_BUFF_LEN);
        can_rec_msg_len = 0;
        break;
    case APP_UPDATE_RECV_DATA:
        app_update_receive_app_data();
        break;
    case APP_UPDATE_CHECK_DATA:
        app_update_check_data();
        break;
    case APP_UPDATE_CHANGE_BOOT_MODE:
        app_update_change_boot_mode();
        break;
    default:
        break;
    }
}
