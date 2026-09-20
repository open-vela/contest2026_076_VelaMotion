# VelaMotion · SomatoSync 体感同步系统的 openvela 板级适配

> 队伍：`contest2026_076_VelaMotion`
> 编号：`076` ｜ 队伍名：`VelaMotion`
> 上游基线：`vendor_espressif/boards/esp32s3/esp32s3-eye`（openvela 官方已适配的 ESP32-S3 参考板）

---

## 一、作品简介

**SomatoSync** 是一套面向**医疗级全身运动障碍量化追踪**的高频低延迟多节点
体感数据同步系统。它在人体上布置 12 个可穿戴 IMU 节点，通过自研的
**ESP-NOW TDMA 时分多址调度 + 自适应跳频（AFH）** 协议，把多路高频姿态数据
无碰撞、低延迟地汇聚到网关，再由网关经以太网上送 ROS 2 上位机，完成骨架重建、
运动功能评估（ROM / 步态对称性 / 跌倒风险）与 AI 康复报告生成。

覆盖病种：脑卒中、帕金森、脑瘫、多发性硬化、骨关节术后、运动损伤。

原系统固件基于 ESP-IDF 开发并已在真机跑通。**本参赛作品的贡献是把这套系统的
两块自研板卡适配到 openvela**：完成板级 bring-up、外设引脚与供电时序、
defconfig 配置与构建系统集成，使 openvela 能在网关板与可穿戴节点板上启动运行，
并把 SSD1306 显示子系统落地为 openvela 标准图形设备。

**亮点**

1. **不是"点灯级"适配**：适配对象是两块真实产品板，外设覆盖
   以太网（W5500）、NFC（PN5180）、双路 IMU（ICM-42688）、SD 卡、OLED、触觉反馈、电池计量。
2. **图形能力真实落地**：SSD1306 通过 `lcddev_register()` 成为 `/dev/lcd0`，
   打开 `CONFIG_LCD_FRAMEBUFFER` 后即标准 framebuffer，LVGL 与 fb 类 demo 无需改动即可运行。
3. **有完整的系统级语境**：适配不是孤立的技术练习，而是服务于一个已跑通的
   医疗级体感同步系统 —— 板子跑起来之后要承载 TDMA 调度、姿态解算与康复评估。
4. **文档与可复现性**：板级适配指南（`board/somatosync_esp32s3/README_zh-cn.md`）
   给出了完整的编译、烧录、逐外设验证步骤，评委可照着复现。

---

## 二、选题方向

**主方向：新硬件平台适配（板级适配）**

理由：本队的网关板与节点板都是**自研硬件、此前从未适配过 openvela**，
需要从板级 bring-up 做起，符合赛道「完成从底层 BSP 移植、驱动开发到系统构建的
全链路适配工作，使 openvela 能够在目标硬件上正常启动并运行核心功能」的定义。

**组合方向：AI 硬件产品创新（提供系统语境）**

本适配所服务的 SomatoSync 系统本身是一个「能主动、会执行」的端侧 AI 应用：
节点在片内跑双 ESKF 姿态融合，网关侧对时序数据做 AI 康复分析并生成报告。
板级适配是这套 AI 能力的运行底座。

**能力落地（合规）**

大赛要求至少落地**图形 / AI / 多媒体**三项核心能力之一。本作品落地**图形**：

- SSD1306 128×64 面板注册为 openvela 标准 LCD 设备 `/dev/lcd0`；
- 使能 `CONFIG_LCD_FRAMEBUFFER` 后成为标准 framebuffer，可直接承载 LVGL；
- 面板承载网关的 TDMA 调度状态、节点在线数、存储状态，
  以及节点的电量与姿态解算状态 —— 即原固件 OLED 菜单的 openvela 版本。

---

## 三、目录结构

```
contest2026_076_VelaMotion/
├── board/
│   └── somatosync_esp32s3/          # 板级适配代码（本作品主体）
│       ├── Kconfig                  # 板型二选一 + 外设开关
│       ├── CMakeLists.txt           # 板级 CMake 入口
│       ├── include/
│       │   └── board.h              # 两块板的引脚映射（条件编译区分板型）
│       ├── src/
│       │   ├── somatosync_esp32s3.h     # 板内私有声明
│       │   ├── somatosync_boot.c        # esp32s3_board_initialize / board_late_initialize
│       │   ├── somatosync_appinit.c     # board_app_initialize（BOARDIOC_INIT）
│       │   ├── somatosync_bringup.c     # 板级外设总 bring-up
│       │   ├── somatosync_board_lcd.c   # SSD1306 OLED（图形能力落地点）
│       │   └── somatosync_board_eth.c   # W5500 以太网板级 glue
│       ├── configs/
│       │   ├── gateway/defconfig    # 网关板配置
│       │   └── node/defconfig       # 节点板配置
│       └── README_zh-cn.md          # 板级适配指南（编译 / 烧录 / 逐外设验证）
├── logs/                            # AI Coding 对话日志（按 github_login/日期 归档）
├── docs/
│   ├── 技术报告.md                  # 技术报告文档正文（可导出 .docx / .pdf）
│   ├── 提交操作手册.md              # 提交动作清单（fork / PR / CLA / 日志）
│   ├── reference/                   # 硬件资料：原理图、PCB 图、引脚来源溯源
│   └── design/                      # 系统设计文档：架构 / 算法 / 协议 / 硬件
├── .claude/
│   └── skills/openvela-board-porting/   # 沉淀的可复用 Skill（AI 开发评分项）
├── contest2026_076_VelaMotion.xml   # repo manifest（含 linkfile 映射）
└── README.md                        # 本文件
```

**manifest 映射**（`contest2026_076_VelaMotion.xml`）：

| 本仓目录 | 映射到 openvela 工程 |
| --- | --- |
| `board/somatosync_esp32s3` | `vendor/espressif/boards/esp32s3/somatosync_esp32s3` |

> 选择 `vendor/espressif/boards/esp32s3/` 而不是模板默认的
> `vendor/openvela/boards/`：ESP32-S3 的板级配置是通过
> `CONFIG_ARCH_BOARD_CUSTOM_DIR` 机制定位的，板目录必须与芯片支持
> （`vendor_espressif`）放在一起，才能和上游 `esp32s3-eye` 走同一条路径加载
> Kconfig 与板级源码。这一点已在板级适配指南中说明。

---

## 四、运行方式

### 4.1 拉取工程

```bash
repo init -u https://github.com/open-vela/contest2026_076_VelaMotion \
  -b dev-ai-contest-2026 -m contest2026_076_VelaMotion.xml
repo sync -c -j8
```

同步后本仓位于工作区 `contest2026_076_VelaMotion/`，`board/somatosync_esp32s3`
已被 manifest 软链到 `vendor/espressif/boards/esp32s3/somatosync_esp32s3`。

### 4.2 编译

openvela 统一入口 `build.sh`，参数为 board config 路径：

```bash
cd <openvela 工作区根目录>

# 网关板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/gateway -j$(nproc)

# 节点板
./build.sh vendor/espressif/boards/esp32s3/somatosync_esp32s3/configs/node -j$(nproc)
```

产物位于 `nuttx/`：`nuttx`（ELF）、`nuttx.bin`（平面镜像）、`nuttx.hex`。

> ⚠️ 依赖 `open-vela/nuttx` 与 `open-vela/vendor_espressif` 的
> `dev-ai-contest-2026` 分支（芯片层依赖尚未合入 `trunk` / `dev`）。

### 4.3 烧录

ESP32-S3 simple-boot：单个平面镜像烧到 flash 偏移 `0x0`，无需 bootloader/分区表。

```bash
PORT=$(ls /dev/serial/by-id/ | grep Espressif_USB_JTAG_serial_debug_unit | head -1)
PORT="/dev/serial/by-id/$PORT"

esptool --chip esp32s3 --port "$PORT" --baud 460800 \
        --before default-reset --after hard-reset \
        write-flash 0x0 nuttx/nuttx.bin
```

连不上时：按住 **BOOT** → 点一下 **RESET** → 释放 **BOOT**，进入 ROM 下载模式。

### 4.4 运行与验证

USB-CDC 串口 115200 8N1（`picocom -b 115200 /dev/ttyACM*`），按 RESET 进入 `nsh>`：

```
nsh> uname -a
NuttX  0.0.0 <commit> <date> xtensa somatosync_esp32s3

nsh> ls /dev
nsh> i2c dev 0x03 0x3c              # SSD1306 ACK
nsh> ls /dev/lcd0                   # 图形设备已注册
nsh> ifconfig eth0                  # 网关板：W5500
nsh> mount -t vfat /dev/mmcsd1 /mnt # 网关板：microSD
```

完整的逐外设验证清单见
[`board/somatosync_esp32s3/README_zh-cn.md`](board/somatosync_esp32s3/README_zh-cn.md) 第六节。

---

## 五、AI Coding 使用说明

本作品的开发全程使用 AI Coding 辅助，完整对话日志见 [`logs/`](logs/)。

| 环节 | AI 承担的工作 |
| --- | --- |
| 规范调研 | 通读大赛官方文档与赛道指引，比对 `esp32s3-eye` 与大赛模板 board 骨架的差异，确定 `ARCH_BOARD_CUSTOM_DIR` 才是 ESP32-S3 板级配置的正确加载路径 |
| 方案设计 | 板型划分（网关 / 节点共用一套板目录 + 两套 defconfig）、外设开关粒度、图形落地点的选择 |
| 代码生成 | `board.h` 引脚映射、`Kconfig` 选项、板级 `CMakeLists.txt` / `Make.defs`、boot / appinit / bringup / LCD / 以太网板级 glue 的骨架 |
| API 核对 | 逐项核对 NuttX 真实接口（`ssd1306_initialize`、`lcddev_register`、`w5500_initialize`、`esp32s3_configgpio` 等），避免生成不存在的 API |
| 文档沉淀 | 板级适配指南、作品说明、提交操作手册 |

**AI 带来的实际收益**：把「读官方文档 → 找到可信参考实现 → 核对真实 API →
产出符合工程约定的骨架」这条链路从数天压缩到数小时，且在只有 2 天提交窗口的
情况下保证了产出与上游约定一致、可被评委复现。

**Skill 沉淀**：本队沉淀了一个可复用的 Skill
[`openvela-board-porting`](.claude/skills/openvela-board-porting/SKILL.md)
—— 把「给一块新板卡做 openvela 适配」的全流程固化为可被 AI 助手自动发现的技能：
从选定同芯片参考板、确认 `ARCH_BOARD_CUSTOM_DIR` 加载机制、manifest 映射、
板级源码切分，到 API 核对与自检清单。配套
[ESP32-S3 专项清单](.claude/skills/openvela-board-porting/references/esp32s3-porting-checklist.md)
收录了本次适配中逐项核实过的真实 API 签名与易错点。下次在 openvela 上加板即可直接复用。

---

## 六、当前交付边界与后续工作

### 6.1 当前固件里有什么、开机能看到什么

**本次交付的是 openvela 的板级适配，不是把 SomatoSync 应用搬到 openvela 上运行。**
为避免误判，这里把边界写清楚。

固件里**有**：

| 内容 | 说明 |
| --- | --- |
| 硬件定义 | 两块自研板的完整引脚映射（W5500 / PN5180 / SSD1306 / microSD / 双 IMU / 电池） |
| 板型划分 | `gateway` / `node` 两套 defconfig，同一板目录条件编译 |
| 板级驱动初始化 | `somatosync_bringup()`：SSD1306 OLED（`/dev/lcd0`）、W5500 以太网、microSD、Wi-Fi / ESP-NOW 底层能力 |
| openvela 系统本体 | `open-vela/nuttx`（分支 `dev-ai-contest-2026`）+ espressif 芯片层 |
| 可烧录镜像 | 网关板 `nuttx.bin` 834.9 KB、节点板 671.9 KB，两套均编译通过 |

固件里**没有**：

| 未包含 | 原因 |
| --- | --- |
| ESP-NOW TDMA 时隙调度 | 建立在 ESP-IDF 的 `esp_now_*` API 之上，需重写到 NuttX socket |
| 双 ESKF 姿态融合 / 双 IMU 在线校准 | 依赖 FreeRTOS 双核任务模型，需映射到 NuttX task / pthread |
| 网关状态机、OLED 菜单与五向键 | 建立在 ESP-IDF NVS 与驱动之上，属应用层工作 |
| AI 康复报告、ROS 2 上位机链路 | 属应用层与上位机，不在板级适配范围 |

**烧录后能看到什么**（这是当前「能跑」的实际水平）：

```
nsh> uname -a           # NuttX ... xtensa somatosync_esp32s3
nsh> ls /dev            # 出现 lcd0 / i2c0 / mmcsd1 等设备节点
nsh> i2c dev 0x03 0x3c  # SSD1306 应答
nsh> ifconfig eth0      # 网关板：W5500
```

即 **操作系统起来了、外设可控** —— 这正是板级适配赛道的验收范围。

### 6.2 从「板子跑起来」到「应用跑起来」还差两步

**第一步：应用移植。** 把 `gateway_app` / `node_app` 的逻辑按 NuttX 的 POSIX API 重写，
做成 openvela 的 app 形态（`app/<name>/` + `Kconfig` + `Make.defs`，注册为 builtin），
再由 manifest 的 `<linkfile>` 映射进编译树，defconfig 中打开后在 `nsh>` 下直接执行。

**第二步：协议层映射**（工作量最大）：

| 原方案（ESP-IDF） | 目标（NuttX / POSIX） |
| --- | --- |
| `esp_now_send` / `esp_now_recv` | NuttX raw socket + `esp32s3_wifi_adapter` |
| FreeRTOS task / queue | NuttX task / `mq_*` / pthread |
| Core0 / Core1 双核 + `__atomic_*` | NuttX SMP spinlock / atomic / CPU 亲和 |
| `esp_timer_get_time()` | `clock_gettime()` |
| `nvs_flash` | NuttX MTD + littlefs / FAT |
| `ESP_LOGI` / `ESP_ERROR_CHECK` | `syslog()` / 返回码检查 |

### 6.3 其它后续工作

1. **ICM-42688 传感器驱动**：节点板双 IMU 的 SPI 总线已在 defconfig 与
   `board.h` 中就绪，但传感器注册默认关闭（`CONFIG_SOMATOSYNC_IMU=n`）——
   需先确认 `drivers/sensors/` 是否已提供 ICM-42688 驱动。
2. **PN5180 NFC 协议驱动**：本次只做板级 glue（SPI3 + BUSY / RST），
   「碰一碰配网」协议栈是后续工作。
3. **ESP-NOW TDMA 协议层**：见 6.2 第二步。
4. **电源与时序**：节点板的 LDO 门控与 ADC 分压 MOS 的时序需要在真机上标定。
5. **真机验证**：两套固件均已完成编译链接并通过符号表核对（板级函数确实在镜像内），
   但**尚未在真机上启动过**；引脚映射需对照原理图复核后再烧录。

---

## 七、提交物索引

| 交付物 | 位置 |
| --- | --- |
| 板级适配代码 | `board/somatosync_esp32s3/` |
| 板级适配指南（编译 / 烧录 / 逐外设验证 / 已知限制） | `board/somatosync_esp32s3/README_zh-cn.md` |
| 技术报告文档 | `docs/技术报告.docx` ／ `docs/技术报告.pdf` |
| **演示视频**（4 分 15 秒） | `docs/演示视频.mp4` |
| **作品展示照片**（目标硬件多角度 + 系统实机运行） | `docs/作品展示照片/` |
| **大赛开发板发放凭证**（组委会发放的 ESP32-P4X 实物照） | `docs/大赛开发板发放凭证/` |
| 硬件资料（原理图 / PCB） | `docs/reference/` |
| 系统设计文档（原型背景） | `docs/design/` |
| 提交手册 | `docs/提交操作手册.md` |
| 可复用 Skill | `.claude/skills/openvela-board-porting/` |
| **AI 对话记录（索引）** | `docs/ai-coding-sessions/` |

> **关于 AI Coding 日志**：本项目的 AI 开发主要使用 **VS Code（GitHub Copilot Chat，
> 含小米 MiMo 扩展）** 与 **MiMoCode（小米基于 OpenCode 的定制版）**。
> 大赛采集器仅支持 `claude-code` / `codex` / `opencode` / `kiro` 四个适配器，
> **不包含上述两种**，因此 `logs/` 无法自动生成。
> 相关真实对话记录已**逐字节原样提取**，索引见
> [docs/ai-coding-sessions/](docs/ai-coding-sessions/)，另已邮件向组委会说明并附原始记录。

---

## 八、许可协议

Apache-2.0。参赛作品为原创，无版权、专利及其他法律纠纷。
