# MiMoCode 会话记录（构建机 192.168.0.***）

## 来源与提取方式

| 项 | 值 |
| --- | --- |
| 来源机器 | `192.168.0.***`（主机名 `zysros2`，Ubuntu 22.04） |
| 数据位置 | `~/.local/share/mimocode/mimocode.db`（SQLite） |
| 提取方式 | 在服务器上用 `sqlite3.backup()` 做**一致性快照**，原样取回 → `raw/mimocode.db` |
| 会话导出 | `sessions/*.jsonl` —— 按会话分组的**原始数据库行**，`data` 字段原样保留，未做任何改写 |
| 可读稿 | `transcripts/*.md` —— 派生产物，便于阅读 |

## 关键事实：用的就是小米 MiMo

| 模型 | 出现消息数 |
| --- | --- |
| **`xiaomi/mimo-v2.5-pro`（小米 MiMo）** | **238** |
| `mimo/mimo-auto`（小米 MiMo，自动档） | 12 |

`providerID` 明确为 `xiaomi` / `mimo`。另见 外发材料中的 `raw/model.json`：

```json
{ "recent": [ { "providerID": "xiaomi", "modelID": "mimo-v2.5-pro" } ],
  "variant": { "mimo/mimo-auto": "default", "xiaomi/mimo-v2.5-pro": "default" } }
```

## 统计

- 会话 **11** 个，消息 **250** 条（41 用户 / 209 助手），部件 **985** 个
- 累计 token：输入 **1,171,411** ／ 输出 **48,754**
- 时间范围：**2026-07-07 ～ 2026-07-27**

## 覆盖范围（**如实说明，重要**）

11 个会话按工作目录分布：

| 工作目录 | 会话数 | 与本参赛作品的关系 |
| --- | --- | --- |
| `/home/yangcong` | 3 | 目录浏览与内部会话（含 `checkpoint-writer`） |
| `/home/yangcong/RK3566/tspi_linux_6.1_sdk` | 4 | 泰山派 RK3566 Linux SDK —— **与本参赛作品无关** |
| `/home/yangcong/mocap_ws` | 4 | 动捕工作区（含 `gateway_simulator.py`）—— **属 SomatoSync 项目** |

> ⚠️ **没有任何一个会话发生在 openvela 工作区（`~/VelaMotion`）。**
>
> 因此这批记录能证明「**使用了小米 MiMo 与 MiMoCode 进行本项目相关开发**」，
> 但**不覆盖** openvela 板级适配本身的工作。

另需说明：其中若干会话是 MiMoCode **自身的内部 agent**（`checkpoint-writer`、
`Auto Dream`、`Manual dream memory consolidation`、`Conversation title request`），
并非人工编码对话；真正的人工会话主要是 `6.1内核设备树设备列表`（98 条消息）与
`启动ROS软件`（16 条消息）。

## 与官方采集器的关系

**MiMoCode 是 OpenCode 的小米定制版**，二进制中可见充分证据：

```
"You are MiMoCode, an interactive CLI tool that helps users with software engineering tasks."
"dialog.plugins.empty":"Plugins configurados en opencode.json"
"provider.connect.opencodeZen...."
user-agent=mimocode/0.1.0        运行时: Bun (B:/~BUN/root/...)
```

官方采集器 `contest-log-collector` 有 **`opencode` 适配器**（插件形式，
部署到 `<demo-repo>/.opencode/plugins/collector.js`，`TOOL_ID = "opencode"`），
但**没有 `mimo` / `mimocode` 适配器** —— 采集器代码里 grep 不到任何 mimo 字样。

因此本机这些 MiMoCode 历史会话**无法被官方工具自动采集**；
本目录是把它们原样提取出来，交由组委会判断。

## 完整性

- `raw/mimocode.db` 为服务器端一致性快照，可独立复核（可用任意 SQLite 客户端打开）
- `sessions/*.jsonl` 每行是 `{table, id, session_id, time_created, data}`，
  其中 `data` 与数据库中存储的 JSON **完全一致**
- 导出脚本 `export-mimocode.py` 保留在工作区根目录，可重复运行复现

## 目录结构

```
ai-coding-sessions/mimocode/
├── README.md          本说明
├── INDEX.md           会话索引（标题/目录/日期/消息数/模型/token）
├── manifest.json      机读索引
├── raw/               原始快照与状态文件
│   ├── mimocode.db           SQLite 一致性快照
│   ├── model.json            模型选择记录（xiaomi / mimo-v2.5-pro）
│   ├── prompt-history.jsonl  用户 prompt 历史（29 条）
│   ├── trusted-workspaces.json
│   └── kv.json
├── sessions/          按会话分组的原始行（JSONL）
└── transcripts/       可读转写稿（派生）
```
