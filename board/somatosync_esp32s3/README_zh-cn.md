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
│   ├── somatosync_board_eth.c    # W5500 以太网板级 glue
│   └── somatosync_board_spi.c    # 芯片层要求的板级 SPI status 实现
├── scripts/                      # configure.sh 定位的构建脚本目录
│   ├── Make.defs                 # 工具链与链接脚本选择（必需）
│   └── patches/
│       └── 0001-esp-hal-3rdparty-fix-spinlock-init.patch
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

### 4.1 首次构建前置条件（实测，缺一不可）

以下四项都是在真机编译过程中**实际踩到并解决**的，不是推测。`esp32s3-eye` 的
官方文档只覆盖了其中一部分，其余是这个工作区首次构建 ESP32-S3 时才会暴露的问题。

**① 把 xtensa 工具链加入 PATH**

`build.sh` **不会**自己 `source build/envsetup.sh`，而工具链的 PATH 正是在那里设置的
（`build/envsetup.sh` 第 1123 行）。更麻烦的是，脚本里那条通用 glob
（`xtensa{,-none}-{eabi,elf}/bin`）**匹配不到** `xtensa-esp32s3-elf` 这个名字，
所以即便 source 了也仍然找不到编译器。必须显式指定：

```bash
export PATH=$PWD/prebuilts/gcc/linux-x86_64/xtensa-esp32s3-elf/bin:$PATH
```

不设的后果很隐蔽：`xtensa-esp32s3-elf-gcc: 未找到命令`，但 `make` 不会立刻退出，
而是继续空转 —— 看起来像"编译很慢"，实际一个目标文件都没产出。

**② 板目录必须有 `scripts/Make.defs`**

`nuttx/tools/configure.sh` 按固定顺序查找 `Make.defs`，其中一条是：

```
src_makedefs=${configpath}/../../scripts/Make.defs
```

即 `<板目录>/scripts/Make.defs`。缺了它配置阶段直接报
`File Make.defs could not be found` 并中止。该文件内容是 ESP32-S3 芯片级通用的
（工具链 defs + 链接脚本选择），与具体板子无关。

**③ 必须手工应用 esp-hal-3rdparty 的 spinlock 补丁**

`nuttx/arch/xtensa/src/esp32s3/Make.defs` 会自行克隆
`espressif/esp-hal-3rdparty` 到 `<工作区>/nxtmpdir/` 缓存再拷进 `chip/`，
但克隆下来停在 `main` 分支，而 `main` 上 `components/esp_hw_support/clk_ctrl_os.c` 的
`LOCK_INITIALIZER_UNLOCKED` 是 `0`，编不过：

```
error: invalid initializer
  static lock_type_t periph_spinlock = LOCK_INITIALIZER_UNLOCKED;
```

补丁把它改成 `SP_UNLOCKED`，本目录已随仓提供：
`scripts/patches/0001-esp-hal-3rdparty-fix-spinlock-init.patch`。

> ⚠️ **注意**：`esp32s3-eye` 的 README 称 `build.sh` 会自动重新应用该补丁，
> 但本工作区版本的 `build.sh` 及全树任何 `.sh` / `.mk` / `CMakeLists.txt` 里
> **都没有这个逻辑**（已逐文件 grep 确认）。所以必须手工执行，且**克隆完成后**
> 才能打（构建系统会重新克隆 `chip/esp-hal-3rdparty`，早打会被覆盖）。

```bash
cd nuttx/arch/xtensa/src/esp32s3/esp-hal-3rdparty
git apply -p1 <板目录>/scripts/patches/0001-esp-hal-3rdparty-fix-spinlock-init.patch
```

**④ 板级必须提供 `esp32s3_spi<N>_status()`**

芯片层的 SPI ops 表引用了 `esp32s3_spi2_status()`，但这个函数**故意不在芯片层实现**
——每块 ESP32-S3 板自己提供（答案取决于板子怎么接片选）。本板由
`src/somatosync_board_spi.c` 提供。缺了会在链接阶段报：

```
undefined reference to `esp32s3_spi2_status'
```

**⑤ `CONFIG_TLS_TASK_NELEM` 必须 > 0**

esp32s3 的 Wi-Fi 适配层调用 `task_tls_alloc()` / `task_tls_get_value()` /
`task_tls_set_value()`，这些函数在 `nuttx/libs/libc/tls/` 下受
`CONFIG_TLS_TASK_NELEM` 保护。本仓两个 defconfig 均已设为 `4`。缺了会在链接阶段报：

```
undefined reference to `task_tls_alloc'
```

**⑥ 两个「默认值」符号必须显式写进 defconfig（真机实测踩到）**

`make savedefconfig` 会把等于 Kconfig 默认值的符号剥掉。以下两个符号一旦缺失，
报错信息**完全不指向根因**，且只在「重新 configure 过、但 olddefconfig 没跑成」的
机器上出现：

| 符号 | 缺失时的表现 | 根因 |
| --- | --- | --- |
| `CONFIG_XTENSA_TOOLCHAIN_ESP=y` | `gcc: error: unrecognized command-line option '-mlongcalls'` | `arch/xtensa/src/lx7/Toolchain.defs` 由它推导 `CROSSDEV`；为空则 `CC` 回退成宿主 `gcc` |
| `CONFIG_STACK_USAGE_WARNING=0` | `gcc: error: missing argument to '-Wstack-usage='` | 同一文件写的是 `ifneq ($(CONFIG_STACK_USAGE_WARNING),0)`，**未定义（空）≠ 0**，于是生成了空参数的 `-Wstack-usage=` |

本仓两套 defconfig 已显式包含这两行。

**⑦ 主机侧工具必须在 PATH 上**

`make olddefconfig` 需要 `kconfig-conf`（来自 `prebuilts/kconfig-frontends/bin`；
若该目录缺失，`configure.sh` 会报 `kconfig-conf: 未找到命令`，`.config` 只展开出
defconfig 里那几十个符号，随后编译必然以 `#error Unknown XTENSA architecture` 失败）。
`nuttx/tools/Unix.mk` 结尾的打包步骤需要 `esptool.py`。

两者都不在本仓范围内，但会让「照着文档做却编不过」——排错时先确认它们存在。

### 4.2 编译命令

openvela 统一入口是工作区根目录的 `build.sh`，参数为 **board config 路径**：

```bash
cd <openvela 工作区根目录>

# 先按 4.1 ① 设置工具链 PATH
export PATH=$PWD/prebuilts/gcc/linux-x86_64/xtensa-esp32s3-elf/bin:$PATH

# 网关板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/gateway -j$(nproc)

# 节点板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/node -j$(nproc)
```

> 首次构建时 `build.sh` 会自动克隆 `esp-hal-3rdparty`（全量克隆，含 ESP-IDF 的
> Wi-Fi 固件等大文件，**需要数分钟到十几分钟**，请耐心等待，不要中断）。克隆完成后
> 再执行 4.1 ③ 的补丁，然后重新运行一次 `build.sh`。

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

## 八、真机验证结果与已知限制

### 8.1 真机验证结果（2026-09-19，网关板）

在 **SomatoSync 网关板**（ESP32-S3-WROOM-1，8 MB Octal PSRAM，16 MB flash）上完成
首次真实硬件烧录与启动验证：

- 芯片识别：`ESP32-S3 (QFN56) revision v0.2`，`Embedded PSRAM 8MB (AP_3v3)`，
  `Detected flash size: 16MB`，MAC `28:84:85:52:c4:0c`
- 烧录：`write-flash 0x0 nuttx.bin`（854,968 B），`Hash of data verified.`
- **结果：openvela 内核成功启动**。串口任务表实测：

  | PID | CPU | 状态 | 栈 | 任务 |
  | --- | --- | --- | --- | --- |
  | 0 | 0 | Assigned | 3040 | CPU0 IDLE |
  | 1 | 1 | Running | 3040 | CPU1 IDLE |
  | 2 | 0 | Waiting Semaphore | 4016 | hpwork |
  | 3 | 0 | **Ready** | 3008 | **nsh_main** |
  | 4 | 0 | Running | 6608 | wifi |

  即双核 SMP 调度正常、工作队列就绪、NSH 已启动。

**真机验证发现的缺陷（已修复）**：`board_lcd_initialize()` 返回 `-EIO`。
对照量产 ESP-IDF 固件（`apps/gateway_app/components/oled_ui/oled_ui.c`：
`OLED_SDA_PIN = 1`、`OLED_SCL_PIN = 2`）发现网关板 I2C 引脚顺序写反了
（曾误写为 `SCL = 1 / SDA = 2`）。已修正 `include/board.h` 与
`configs/gateway/defconfig`。**该缺陷只在真机上暴露** —— 编译、链接与符号核对全部通过。

### 8.2 已知问题（尚未解决）

**Wi-Fi 适配层在真机上 panic**。时间线：`t=0.75 s` OLED 初始化 →
`t=3.63 s` Wi-Fi user panic：

```
_panic: User Exception: EXCCAUSE=001c task: wifi
[CPU0] dump_assert_info: Assertion failed user panic ... task(CPU0): wifi
```

`EXCCAUSE=001c` 为 LoadProhibited。经 `addr2line` 解析符号，崩溃点位于
`esp-hal-3rdparty/components/wpa_supplicant/esp_supplicant/src/crypto/crypto_mbedtls.c:93`
的 `sha256_vector` —— 属 **esp-hal 上游无线适配层**，非本板级代码。
panic 后系统进入 halt，因此**当前镜像还不能交付一个可交互的 `nsh>` 会话**。

### 8.3 明确不在本次范围内的部分

1. **三个 chip/vendor 层钩子的真机确认**（源码中以 `NOTE(verify)` 标注）：
   - `board_wlan_init()`：**已确认真机上 panic**（见 8.2）；
   - `board_sdmmc_initialize()`：`esp32s3_board_sdmmc.h` 的归属与签名待确认；
   - `ESP32S3_PIN2IRQ()`：W5500 INTn 的引脚→IRQ 映射宏名待确认。
   三者均按上游 `esp32s3-eye` 的调用方式书写。
2. **ICM-42688 传感器驱动**：节点板双 IMU 的 SPI 总线已在 defconfig 与
   `board.h` 中就绪，但传感器注册默认关闭
   （`CONFIG_SOMATOSYNC_IMU=n`）—— 需先确认 `drivers/sensors/` 是否
   已提供 ICM-42688 驱动，没有则需要一并提交驱动。
3. **PN5180 NFC 协议驱动**：本次只做板级 glue（SPI3 + BUSY/RST），
   「碰一碰配网」协议栈是后续工作。
4. **ESP-NOW TDMA 协议层**：原固件的 `esp_tdma_mac` 组件尚未移植。
5. **电源与时序**：节点板的 LDO 门控与 ADC 分压 MOS 的时序需要在真机上
   标定，避免静态漏电。
6. **ESP32-P4 芯片层移植**：大赛发放的是 ESP32-P4X-Function-EV-Board
   （官方《支持的硬件平台》「待适配开发板」第 1 项），但 openvela 目前
   **没有 ESP32-P4 的芯片层支持**（`arch/risc-v/src/esp32p4` 不存在），
   属从零开始的 RISC-V 芯片层移植，列为后续工作。
7. **defconfig 校正**：本次已用 `olddefconfig` 展开后的结果核对过关键项；
   仍建议在真机上执行 `make menuconfig` + `make savedefconfig` 后回写
   `configs/*/defconfig`。

---

## 九、许可协议

本目录下所有文件均使用 Apache-2.0 协议（SPDX 标识符 `Apache-2.0`），
详见各文件头部声明。符合大赛对参赛作品的开源许可要求。
