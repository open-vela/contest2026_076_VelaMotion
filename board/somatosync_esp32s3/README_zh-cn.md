# SomatoSync 自定义 ESP32-S3 板对 openvela 的支持

[ [简体中文](README_zh-cn.md) ]

本目录是 **VelaMotion 队（`contest2026_076_VelaMotion`）** 为两块自研 ESP32-S3
板卡提供的 openvela 板级适配：**SomatoSync 网关板**与 **SomatoSync 可穿戴节点板**。
两块板来自同一个体感同步系统（ESP-NOW TDMA 高频低延迟多节点 BSN），
原固件基于 ESP-IDF，本目录把它们带到 openvela 上运行。

> **上游基线**：本目录的工程约定对齐 `vendor_espressif/boards/esp32s3/esp32s3-eye`
> —— 该板是本届大赛官方已适配的 ESP32-S3 参考板，也是本适配唯一可信的
> 「如何在一块 ESP32-S3 板子上启动 openvela」范例。

---

## 一、为什么是「板级适配」

ESP32-S3 的**芯片层**（Xtensa 双核、Wi-Fi、BLE、SPI/I2C/SDMMC、USB-Serial）
openvela 已经支持；本队的两块板是**尚未适配过的自有硬件**，其板级差异集中在：

| 差异点 | 网关板 | 节点板 |
| --- | --- | --- |
| 以太网 | W5500（SPI2） | 无 |
| NFC | PN5180（SPI3） | 仅 NFC 唤醒信箱 GPIO18 |
| 显示 | SSD1306 128×64（I2C0） | SSD1306 128×64（I2C0，带电源门控） |
| 存储 | microSD（SDMMC 1-bit） | 无 |
| 传感 | 无 | 双 ICM-42688（SPI2 + SPI3，独立 LDO 与外部 32.768 kHz 时钟） |
| 触觉 | 无 | DRV2605（I2C0） |
| 电池 | 无 | 分压 + 接地 MOS 门控（ADC1_CH1） |

因此本适配的工作量落在 **板级 bring-up、外设引脚与供电时序、defconfig 配置**
上，而不是从零移植一个新架构。

---

## 二、目录结构

```
board/somatosync_esp32s3/
├── Kconfig                       # 板级选项（板型二选一 + 各外设开关）
├── CMakeLists.txt                # 板级 CMake 入口
├── include/
│   └── board.h                   # 两块板的引脚映射（按板型条件编译）
├── src/
│   ├── CMakeLists.txt / Make.defs
│   ├── somatosync_esp32s3.h      # 板内私有函数声明（与参考板同布局，置于 src/）
│   ├── somatosync_boot.c         # esp32s3_board_initialize() / board_late_initialize()
│   ├── somatosync_appinit.c      # board_app_initialize()（BOARDIOC_INIT）
│   ├── somatosync_bringup.c      # 板级外设总 bring-up
│   ├── somatosync_board_lcd.c    # SSD1306 OLED（图形能力落地点）
│   └── somatosync_board_eth.c    # W5500 以太网板级 glue
└── configs/
    ├── gateway/defconfig         # 网关板
    └── node/defconfig            # 节点板
```

经 manifest 中的 `<linkfile>` 映射到 openvela 工程：

```
vendor/espressif/boards/esp32s3/somatosync_esp32s3/   ← 本目录
```

映射后即可用 `CONFIG_ARCH_BOARD_CUSTOM_DIR` 指向它，与 `esp32s3-eye` 的机制一致。

---

## 三、引脚映射

> 下表转写自原 ESP-IDF 固件：网关板取自 `apps/gateway_app/components/`
> （`eth_w5500` / `nfc_pn5180` / `oled_ui` / `sd_card`），节点板取自
> `apps/node_app/components/board_config/include/board_config.h`。
> 原固件公开可查，便于逐条对照：https://github.com/yangcong-bit/SomatoSync-Demo
>
> **烧录前必须对照原理图复核**：`docs/reference/SCH_主机_2026-05-22.pdf`、
> `docs/reference/PCB_PCB1_2026-05-22.pdf`。

### 3.1 网关板（SomatoSync gateway）

| 功能 | 外设 | 引脚 |
| --- | --- | --- |
| 控制台 | USB-CDC | `ttyACM0`，115200 8N1 |
| I2C0 | SSD1306 OLED 128×64 @400 kHz，7 位地址 `0x3c` | SDA 1 / SCL 2 |
| SPI2 | WIZnet W5500 | MOSI 11 / MISO 12 / SCLK 13 / CS 14；INT 10，RST 9 |
| SPI3 | NXP PN5180（NFC） | SCK 42 / MISO 41 / MOSI 40；BUSY 38，RST 47 |
| SDMMC | microSD，1-bit，D3 拉高防止进入 SPI 模式 | CLK 7 / CMD 6 / D0 5 / D3 4 |

### 3.2 节点板（SomatoSync wearable node）

| 功能 | 外设 | 引脚 |
| --- | --- | --- |
| 控制台 | USB-CDC | `ttyACM0`，115200 8N1 |
| I2C0 | SSD1306 OLED + DRV2605 触觉驱动 | SCL 37 / SDA 38 |
| OLED 电源 / 复位 | 面板电源门控与复位 | PWR_EN 35 / RES 36 |
| 电池检测 | 分压接地 MOS 门控 + ADC1_CH1 | GATE 1 / ADC 2 |
| IMU 供电 | IMU-A / IMU-B 独立 LDO 使能 | 4 / 5 |
| IMU 时钟 | 外部 32.768 kHz 注入 | CLKIN 11 |
| SPI2 | ICM-42688（IMU-A） | CS 6 / SCLK 7 / MOSI 8 / MISO 9；INT 10 |
| SPI3 | ICM-42688（IMU-B） | CS 16 / SCLK 15 / MOSI 14 / MISO 13；INT 12 |
| NFC | 唤醒信箱 | GPO 18 |
| 充电检测 | 硬件 V2 预留 | CHG 21 |

---

## 四、编译

openvela 统一入口是工作区根目录的 `build.sh`，参数为 **board config 路径**：

```bash
cd <openvela 工作区根目录>

# 网关板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/gateway -j$(nproc)

# 节点板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/node -j$(nproc)
```

产物在 `nuttx/` 下：`nuttx`（ELF）、`nuttx.bin`（平面镜像）、`nuttx.hex`。

> ⚠️ **分支依赖**：ESP32-S3 的芯片层依赖只在 `open-vela/nuttx` 与
> `open-vela/vendor_espressif` 的 `dev-ai-contest-2026` 分支上齐备。
> `trunk` / `dev` 分支**无法编译**。这与 `esp32s3-eye` 的约束相同。

---

## 五、烧录

本板沿用 ESP32-S3 的 **simple-boot** 方案：单个平面镜像烧到 flash 偏移 `0x0`，
不需要单独的 bootloader 与分区表。

```bash
PORT=$(ls /dev/serial/by-id/ | grep Espressif_USB_JTAG_serial_debug_unit | head -1)
PORT="/dev/serial/by-id/$PORT"

# 可选：仅在切换不同固件家族时使用
esptool --chip esp32s3 --port "$PORT" erase-flash

esptool --chip esp32s3 --port "$PORT" --baud 460800 \
        --before default-reset --after hard-reset \
        write-flash 0x0 nuttx/nuttx.bin
```

若 `esptool` 连不上：按住 **BOOT** → 点一下 **RESET** → 释放 **BOOT**，进入 ROM 下载模式。

---

## 六、首次启动与验证

USB-CDC 串口，115200 8N1：

```bash
picocom -b 115200 /dev/ttyACM*
```

按 RESET 后应看到 NuttX 启动，进入 `nsh>`：

```
nsh> uname -a
NuttX  0.0.0 <commit> <date> xtensa somatosync_esp32s3
nsh> ls /dev
```

按板型逐项验证：

**网关板**

```
i2c dev 0x03 0x3c        # SSD1306 应当在 0x3c 处 ACK
ifconfig eth0            # W5500 的 MAC / IP
renew eth0               # DHCP（若网络里有 DHCP 服务器）
mount -t vfat /dev/mmcsd1 /mnt   # microSD
ping -c 4 <host>         # 以太网联通性
```

**节点板**

```
i2c dev 0x03 0x3c        # SSD1306
i2c dev 0x03 0x5a        # DRV2605（7 位地址 0x5a，需确认）
```

**图形能力（两块板通用）**

```
ls /dev/lcd0             # SSD1306 已注册为 LCD 字符设备
# 若启用 CONFIG_LCD_FRAMEBUFFER，还会出现 /dev/fb0
```

---

## 七、图形能力落地点（合规说明）

大赛要求「项目须使用 openvela 提供的系统能力，且至少落地**图形 / AI / 多媒体**
三项核心能力之一」。本适配的落地点是**图形**：

- SSD1306 128×64 面板通过 `board_lcd_initialize()` + `lcddev_register(0)`
  注册为标准 NuttX LCD 设备，暴露 `/dev/lcd0`；
- 打开 `CONFIG_LCD_FRAMEBUFFER` 后进一步成为标准 framebuffer，
  因此 **LVGL、fb 类 demo 无需改动即可运行**；
- 面板内容与系统功能对应：网关板显示 TDMA 调度状态、节点在线数与
  存储状态；节点板显示电量与姿态解算状态 —— 即原固件 OLED 菜单的
  openvela 版本。

> 后续可在同一 framebuffer 上接入实时姿态可视化，作为决赛的功能增强。

---

## 八、已知限制与后续工作

本次提交是**可编译、可启动、可验证基础外设**的板级适配基线。以下部分明确
不在本次范围内，或需要在真机上进一步确认：

1. **需要真机确认的三个 chip/vendor 层钩子**（源码中以 `NOTE(verify)` 标注）：
   - `ESP32S3_PIN2IRQ()`：W5500 INTn 的引脚→IRQ 映射宏名，
     见 `arch/xtensa/src/esp32s3/`；
   - `board_sdmmc_initialize()`：`esp32s3_board_sdmmc.h` 的归属与签名；
   - `board_wlan_init()`：`esp32s3_board_wlan.h` 的归属与签名。
   三者均按上游 `esp32s3-eye` 的调用方式书写，若头文件名或宏名不同，
   各为一行修正。
2. **ICM-42688 传感器驱动**：节点板双 IMU 的 SPI 总线已在 defconfig 与
   `board.h` 中就绪，但传感器注册默认关闭
   （`CONFIG_SOMATOSYNC_IMU=n`）—— 需先确认 `drivers/sensors/` 是否
   已提供 ICM-42688 驱动，没有则需要一并提交驱动。
3. **PN5180 NFC 协议驱动**：本次只做板级 glue（SPI3 + BUSY/RST），
   「碰一碰配网」协议栈是后续工作。
4. **ESP-NOW TDMA 协议层**：原固件的 `esp_tdma_mac` 组件尚未移植。
   本次适配保证 Wi-Fi/ESP-NOW 的底层能力可用，协议层需另行移植。
5. **电源与时序**：节点板的 LDO 门控与 ADC 分压 MOS 的时序需要在真机上
   标定，避免静态漏电。
6. **defconfig 校正**：提交前请在真机上执行 `make menuconfig` +
   `make savedefconfig`，把结果回写 `configs/*/defconfig`，使文件与
   实际构建一致。

---

## 九、许可协议

本目录下所有文件均使用 Apache-2.0 协议（SPDX 标识符 `Apache-2.0`），
详见各文件头部声明。符合大赛对参赛作品的开源许可要求。
