# STM32 Bootloader 工程

基于 **STM32F103ZET6** 的串口 Bootloader 及配套验证工程，支持通过 UART 串口下载应用程序、
通过外部 Flash（W25Q32）/EEPROM（W24C02）保存升级标志，并带按键触发恢复出厂设置。

---

## 一、硬件平台

| 项目 | 说明 |
|------|------|
| 主控 MCU | STM32F103ZET6（Cortex-M3，512KB Flash / 64KB SRAM） |
| 系统时钟 | HSE 8MHz → PLL×9 → **72MHz**（FLASH_LATENCY_2） |
| 串口升级 | USART1，PA9(TX)/PA10(RX)，波特率 **9600**，异步，空闲中断（ReceiveToIdle_IT）接收 |
| I2C 存储 | I2C2，PB10(SCL)/PB11(SDA)，外接 **W24C02**（2Kbit EEPROM），存升级标志/校验密钥 |
| SPI 存储 | SPI1，PA5(SCK)/PA6(MISO)/PA7(MOSI)，主模式 9MBit/s，外接 **W25Q32**（4MB SPI Flash），存固件镜像 |
| 按键 | key1 = **PF8**，下降沿外部中断（EXTI8），触发恢复出厂（BOOT_RESET） |
| LED | P01 工程中 LED1 = **PA0**（GPIO 输出，上电默认点亮） |
| 开发工具 | STM32CubeMX 6.10.0 生成工程 + **Keil MDK-ARM**（ARMCC）编译 |

---

## 二、工程目录结构

本仓库是一个 **Git 顶层仓库**，下含三个独立的 CubeMX/Keil 子工程：

```
bootloader/                         ← Git 仓库根（.git 在此）
├── README.md
├── P01_led1/                       ← LED 跑通验证工程（GPIO 输出）
│   ├── P01_led1.ioc
│   ├── Core/                       ← HAL 生成的 Src/Inc
│   ├── Drivers/                    ← CMSIS + STM32F1xx HAL 库
│   └── MDK-ARM/                    ← Keil 工程（.uvprojx）
├── p02_bootloader/                 ← ★ 主 Bootloader 工程（核心）
│   ├── p02_bootloader.ioc
│   ├── Core/                       ← main.c、HAL 外设初始化
│   ├── Drivers/
│   └── MDK-ARM/
│       ├── p02_bootloader.uvprojx  ← Keil 工程文件
│       ├── application/            ← app_bootloader.c/h（业务/状态机）
│       └── Interface/              ← Int_bootloader / Int_w24c02 / Int_w25q32
└── p03_bootloade_reset/            ← 复位 / 恢复出厂变体工程（开发中）
    ├── p03_bootloade_reset.ioc
    ├── Core/
    └── MDK-ARM/                    ← 含 Interface/Int_bootloader（reset 分支）
```

> ⚠️ Git 仓库根是 `bootloader/`（顶层 `.git`），不是某个子目录。
> 用 VSCode / Keil 打开子工程时，Git 仍认顶层仓库，分支跟随顶层当前分支。

---

## 三、Flash 内存映射（p02_bootloader）

| 地址 | 区域 | 大小 | 说明 |
|------|------|------|------|
| `0x0800 0000` ~ `0x0800 3FFF` | Bootloader（B 区） | 16KB (0x4000) | 启动先进入此处 |
| `0x0800 4000` ~ `0x0807 FFFF` | 应用程序 A 区 | 496KB (0x7C000) | 串口/外部 Flash 下载的目标 |
| `0x2000 0000` | SRAM 栈顶 | 64KB | 跳转前校验 MSP 是否落在此区间 |

代码宏定义：`App_Address = 0x08004000`、`STACK_ADDR = 0x20000000`、`STACK_END = 0x08080000`。

---

## 四、Bootloader 启动流程（状态机）

`main.c` 初始化外设后按以下顺序执行（`app_bootloader.c`）：

1. **`App_bootloader_check_update()`** — 读 W24C02（`CHECK_UPDATE_ADDR=0x10`）3 字节，
   校验密钥（`CHECK_KEY=0x5A6B`，存在 `0x11`）。密钥不对则复位为 `BOOT_NO_UPDATE`；
   对则取出更新状态：
   - `BOOT_NO_UPDATE = 0x00` — 不更新
   - `BOOT_UPDATE    = 0x01` — 需要更新
   - `BOOT_RESET     = 0x03` — 恢复出厂（由 key1 按键中断置位）
2. **`App_bootloader_check_default()`** — 默认上电延时（当前 5s 占位）。
3. **`App_bootloader_update()`** — 根据状态执行升级 / 跳过的占位逻辑。
4. **`App_bootloader_jump_app()`** → `Int_Bootloader_jump_app()` — 校验 App 栈顶与复位向量，
   关闭中断/SysTick、重映射 `SCB->VTOR = App_Address`、设置 MSP 后跳转到 `0x08004000`。

---

## 五、固件升级（UART 串口下载）

接收逻辑在 `Interface/Int_bootloader.c` 的 `HAL_UARTEx_RxEventCallback`：

- 使用 **USART1 空闲中断 + `HAL_UARTEx_ReceiveToIdle_IT`** 一帧一帧接收 PC 发来的 `.bin`。
- 接收缓冲 512 字节；每收到一帧：
  1. `Int_flash_erase()` — 若该页未全 `0xFF` 则按页擦除（FLASH_TYPEERASE_PAGES）；
  2. `Int_flash_write_halfword()` — 以 **半字（16bit）** 写入 `App_Address + 偏移`。
     > 注意：原始 `.bin` 为大端，STM32 内部 Flash 小端，写入时自动交换字节顺序；
     > 收发字节数为奇数时由 `last_byte` 缓存末字节与下一帧拼接。
- 写入偏移 `bootloarder_offset` 随帧累加，直到整包接收完毕。
- 结束后由状态机跳转至应用程序。

> 备注：当前 USART 波特率为 9600。代码注释指出高波特率串口易丢字节，
> 如需提速，建议在 `Int_bootloader.c` 顶部调高 `USART1.BaudRate` 并配合上位机重传/校验协议。

---

## 六、存储外设驱动

| 文件 | 外设 | 功能 |
|------|------|------|
| `Interface/Int_w24c02.c/.h` | W24C02（I2C EEPROM 2Kbit） | 读写单/多字节，存升级标志与校验密钥（页大小 16B，I2C 地址 `0xA0`） |
| `Interface/Int_w25q32.c/.h` | W25Q32（SPI Flash 4MB） | 读 ID、读/写数据、扇区擦除、写使能，作外部固件镜像存储 |
| `Interface/Int_bootloader.c/.h` | 升级核心 | 串口接收、Flash 擦写、跳转 App、外部擦除接口 |

---

## 七、开发环境与编译

1. 用 **STM32CubeMX 6.10.0** 打开对应 `.ioc` 可重新生成 `Core/` 与外设初始化。
2. 用 **Keil MDK-ARM** 打开各子工程下的 `*.uvprojx` 编译。
3. 下载/调试：
   - Bootloader 烧录到 `0x0800 0000`；
   - 应用程序需将 Keil 的 **Target → IROM1 起始地址改为 `0x08004000`**，烧录到 A 区。

---

## 八、代码模块说明（p02_bootloader）

```
Core/Src/
├── main.c              # 入口：初始化 → 检查更新 → 检查默认 → 升级 → 跳转
├── gpio.c/.h           # LED / key1 引脚
├── usart.c/.h          # USART1（9600）配置
├── i2c.c/.h            # I2C2 配置（W24C02）
├── spi.c/.h            # SPI1 配置（W25Q32）
├── stm32f1xx_it.c      # 中断（EXTI9_5 / USART1）
└── stm32f1xx_hal_msp.c # HAL MSP 初始化

MDK-ARM/
├── application/
│   ├── app_bootloader.c/h   # 升级状态机、EEPROM 标志、按键回调、跳转封装
└── Interface/
    ├── Int_bootloader.c/h    # 串口接收 + Flash 擦写 + 跳转（核心）
    ├── Int_w24c02.c/h        # I2C EEPROM 驱动
    └── Int_w25q32.c/h        # SPI Flash 驱动
```

---

## 九、已知问题 / 待完善（TODO）

- [ ] `App_bootloader_update()` 中「将 W25Q32 镜像写入内部 Flash」逻辑仍为 TODO 占位；
      目前真正下载路径是 **UART 直写 A 区**，W25Q32 镜像复制流程未接上。
- [ ] `App_bootloader_jump_app()` 的 A 区 / 出厂默认区（0x08004000）跳转分支为占位，
      尚未按 `BOOT_RESET` 区分跳转地址。
- [ ] `Int_Bootloader_jump_app()` 中栈顶校验运算符优先级存在隐患
      （`app_stack_ptr & 0xffff0000 != STACK_ADDR` 实际先算 `!=`），建议加括号修正。
- [ ] 编译产物（`.o` / `.crf` / `.hex` / `.uvguix.*` / `.vscode` 日志）当前一并入库，
      建议补充 `.gitignore` 只保留源码与工程文件。
- [ ] 串口升级无完整性校验（CRC/长度帧），正式使用建议加上位机协议与重传。

---

## 十、Git 仓库说明

| 项 | 值 |
|----|----|
| 仓库根 | `bootloader/`（顶层 `.git`） |
| 远程名 | `bootloader` |
| 远程地址 | `https://github.com/XIAOMA-AA/STM32Bootloader.git` |
| 默认分支 | `bootloader` |

> 历史记录：曾因 `p02/p03` 的 `MDK-ARM` 内残留嵌套 `.git` 被 GitHub 识别为子模块（打不开），
> 已移除嵌套仓库并将目录拍平为普通文件夹，现可正常浏览。


can通讯上位机与下位机
      上位机负责发送数据，下位机负责接收数据
      上位机接收指令，比如接收id为1的消息