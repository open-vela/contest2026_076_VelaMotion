# docs/reference/ — 硬件参考资料

本目录存放板级适配所依赖的硬件资料，方便评委核对引脚映射与硬件设计。

| 文件 | 用途 |
| --- | --- |
| `SCH_主机_2026-05-22.pdf` | 网关板原理图 —— **引脚映射的复核依据** |
| `PCB_PCB1_2026-05-22.pdf` | PCB 布局图 |

> `board/somatosync_esp32s3/include/board.h` 中的全部引脚号都转写自原
> ESP-IDF 固件，**必须对照上面的原理图复核后才能烧录**。

## 引脚映射的来源与可核验性

两块板的引脚号来自本队已跑通的原固件，公开可查，便于评委逐条对照：

| 板 | 来源文件（原固件仓） |
| --- | --- |
| 网关板 | `apps/gateway_app/components/` 下 `eth_w5500/eth_w5500.c`、`nfc_pn5180/nfc_pn5180.c`、`oled_ui/oled_ui.c`、`sd_card/sd_card.c` |
| 节点板 | `apps/node_app/components/board_config/include/board_config.h` |

原固件仓（public，供溯源）：https://github.com/yangcong-bit/SomatoSync-Demo

> 该仓为 **ESP-IDF** 工程，是本队作品的原型实现；本次参赛的交付物是
> 把它适配到 openvela 的板级代码，位于本仓 `board/somatosync_esp32s3/`。
> 原固件仓不作为本次参赛的提交内容。

## 系统设计文档

系统级设计（架构 / 算法 / 协议 / 硬件）见 [`../design/`](../design/)：

| 文件 | 内容 |
| --- | --- |
| `系统架构总览与全局框架图.md` | 五层技术栈、全局框架 |
| `双ESKF姿态估计数学推导.md` | 双 IMU 融合算法推导 |
| `软件引擎与事件驱动状态机.md` | 网关 / 节点状态机设计 |
| `系统网络与通信协议规范.md` | TDMA 时隙、AFH 跳频、帧格式 |
| `数据链路与数据形态规范.md` | 数据形态与链路约定 |
| `硬件排雷与故障分析报告.md` | 硬件调试记录 |

多媒体资料（演示视频、PPT）按大赛要求单独提交，不放本仓。
