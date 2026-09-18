# docs/design/ — SomatoSync 原型系统设计文档

本目录是 **SomatoSync 原型系统**（ESP-IDF 版本）的设计文档，作为本次 openvela
板级适配的**背景与语境**一并提交，便于评委理解"这块板子跑起来之后要承载什么"。

## 文档构成

| 文件 | 内容 |
| --- | --- |
| `系统架构总览与全局框架图.md` | 五层技术栈、全局框架 |
| `双ESKF姿态估计数学推导.md` | 双 IMU 融合算法推导 |
| `软件引擎与事件驱动状态机.md` | 网关 / 节点状态机设计 |
| `系统网络与通信协议规范.md` | TDMA 时隙、AFH 跳频、帧格式 |
| `数据链路与数据形态规范.md` | 数据形态与链路约定 |
| `硬件排雷与故障分析报告.md` | 硬件调试记录 |

## ⚠️ 关于文中的交叉引用路径

这些文档里的 `[依据]` / `相关文件` 大量指向 `components/`、`apps/`、
`somatosync_ros2/`、`docs/architecture/` 等路径 —— 它们属于**本队的原型工程仓**，
**不在本参赛仓内**，因此在本仓中点不开。

原型工程仓（public，供溯源）：https://github.com/yangcong-bit/SomatoSync-Demo

## 本参赛仓到底包含什么

| 内容 | 是否属于本次参赛作品 |
| --- | --- |
| `board/somatosync_esp32s3/` openvela 板级适配 | ✅ **本次参赛作品**（含 Kconfig / CMake / boot / appinit / bringup / OLED / W5500 glue / 两套 defconfig） |
| `README.md`、`docs/作品介绍.md`、`docs/提交操作手册.md` | ✅ 本次撰写 |
| `docs/reference/` 原理图与 PCB 图 | 本队自研硬件资料（引脚映射复核依据） |
| `docs/design/` 本目录 | 原型系统背景文档 |
| 原型工程仓的 ESP-IDF 固件与 ROS 2 上位机 | ❌ **不在本仓**，仅作溯源链接 |

**本次参赛作品的贡献**：把 SomatoSync 的两块自研板卡适配到 openvela —— 完成板级
bring-up、外设引脚与供电时序、defconfig 与构建系统集成，并把 SSD1306 显示子系统
落地为 openvela 标准图形设备。

## 原创性

本目录全部文档均为本队原创撰写，不含第三方版权材料。原理图与 PCB 图为本队自研
硬件设计。
