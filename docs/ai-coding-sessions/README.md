# AI 对话记录（AI Coding 佐证材料）

本目录保存本项目开发过程中**真实产生**的 AI 对话记录索引用以说明 AI Coding 的实际使用情况。

## 一、为什么没有官方格式的 `logs/`

大赛日志采集器内置的适配器只有四个：`claude-code`、`codex`、`opencode`、`kiro`。

本项目实际使用的两个 AI 编码工具**都不在支持列表内**：

| 工具 | 说明 | 采集器是否支持 |
| --- | --- | --- |
| VS Code + GitHub Copilot Chat（含小米 MiMo 扩展 `sdmapvstool.xiaomimimo-for-copilot`） | 主力开发环境 | ✗ |
| MiMoCode v0.1.0（小米基于 OpenCode 定制的 CLI） | 构建机上的命令行开发 | ✗ |

因此 `logs/` **无法自动生成**官方格式记录。我们没有把这些记录改写成
`claude-code` / `opencode` 的格式充数——那属于伪造，且会被官方校验脚本按
工具标识与格式一致性检出。本目录的做法是：**逐字节原样提取**真实记录，
并另函向组委会说明情况。

## 二、证据一：VS Code / GitHub Copilot Chat

- 会话文件 **46** 个（其中 22 个含有效对话），合计 **56.1 MB**
- 对话轮数 **258**
- 累计 token：输入 **116,720,158**，输出 **4,561,314**
- 来源 `C:\Users\ZYS20\AppData\Roaming\Code\User\workspaceStorage`
  （按工作区名 `somato|openvela|vela` 过滤）
- 提取方式：**逐字节原样复制**，未修改任何内容

**小米 MiMo 的使用情况**：`mimo-v2.5-pro` 出现在 **18** 个会话、`mimo-v2.5` 出现在 **1** 个会话，
由小米官方 Copilot 扩展 `sdmapvstool.xiaomimimo-for-copilot` 提供。

其它模型：`deepseek-v4-flash`(10)、`glm-5.2`(9)、`auto`(9)、`deepseek-flash`(2)、
`claude-haiku-4.5`(1)、`claude-haiku-4-5-20251001`(1)。

> VS Code 会话文件是「快照 + 增量补丁」格式：先写整份 `{"kind":0,"v":{...}}` 快照，
> 再追加以 `{"kind":1,"k":[...],"v":...}` 表示的补丁，补丁既可整条追加请求
> （`k` 长度为 2），也可只改某个字段。索引对两种形态都统计，并对嵌套结构做有界递归兜底。

明细见 [`INDEX.md`](INDEX.md)，机器可读清单见 [`manifest.json`](manifest.json)。

## 三、证据二：MiMoCode（构建机）

- 会话 **11** 个，消息 **250** 条，部件 **985** 个
- 累计 token：输入 **1,171,411**，输出 **48,754**
- 模型：`xiaomi/mimo-v2.5-pro` **238** 条消息、`mimo/mimo-auto` **12** 条消息
- 存储位置：`~/.local/share/mimocode/mimocode.db`（SQLite）

MiMoCode 是 OpenCode 的分支，判据来自记录本身：会话中出现
`"You are MiMoCode, an interactive CLI tool..."`、配置文件 `opencode.json`、
`provider.connect.opencodeZen`、Bun 运行时、`user-agent=mimocode/0.1.0`。
注意两者存储机制不同——OpenCode 用文件存储，MiMoCode 用 SQLite。

> **如实说明工作目录分布**：这 11 个会话的目录为 `~`(3 个)、
> `~/RK3566/tspi_linux_6.1_sdk`(4 个)、`~/mocap_ws`(4 个)。
> 其中 `~/mocap_ws` 是 SomatoSync 原型（算法 / 仿真）工作区，
> `RK3566` 为通用 Linux 内核工作；**构建机的 `~/VelaMotion` 目录下没有 MiMoCode 会话**。
> 也就是说 MiMoCode 主要服务于原型与构建机侧工作，
> 本次 openvela 板级适配本身主要依靠第二节的 VS Code + Copilot Chat 完成。

明细见 [`mimocode/INDEX.md`](mimocode/INDEX.md)。

## 四、目录内容

```
docs/ai-coding-sessions/
├── README.md            本文件
├── INDEX.md             VS Code / Copilot Chat 会话索引（46 个会话）
├── manifest.json        机器可读清单（含每个文件的 SHA256 与统计）
├── mimocode/
│   ├── README.md        MiMoCode 导出说明
│   ├── INDEX.md         MiMoCode 会话索引（11 个会话）
│   ├── manifest.json    机器可读清单
│   └── transcripts/     11 份可读对话记录（Markdown）
└── tools/
    ├── extract-ai-sessions.py   VS Code 会话提取脚本（可复现）
    └── export-mimocode.py       MiMoCode SQLite 导出脚本（可复现）
```

> 完整原始记录（VS Code 的 46 个 `.jsonl` 与 MiMoCode 的 `mimocode.db` 等，
> 共 81 个文件 / 60.9 MB）体积较大且含真实密钥与内网信息，**不随仓库提交**，
> 以离线包形式单独提供，见下节。

## 五、外发材料与脱敏披露

| 包 | 内容 | 大小 | SHA256 | 用途 |
| --- | --- | --- | --- | --- |
| `VelaMotion_AI对话记录_外发脱敏版_20260919.zip` | 全量 81 个文件，扣除下表披露的替换 | 12.1 MB | `08F0CAF737D20D30CD89BF1A1EBEE226F78A71FBB96CE7514B5B8A4CADBE4318` | **提交 / 发送组委会** |
| `VelaMotion_AI对话记录_未脱敏_请勿外传_20260919.zip` | 全量 81 个文件，逐字节原件 | 12.1 MB | `33A310FBCF787424D6AB3DFABE970F02D2484A8F1D93D3FE54315C88976D05E7` | 自留，不分发 |

**脱敏内容完全披露**（仅 1 类，共 90 处，涉及 4 个文件）：

- 替换对象：真实 **DeepSeek API Key**（来自原项目
  `apps/gateway_app/components/ai_analyzer/ai_analyzer.c` 中的硬编码 `AI_DEFAULT_KEY`）
- 替换为：`<REDACTED-DEEPSEEK-API-KEY>`
- 分布：

| 文件 | 处数 |
| --- | --- |
| `sessions/c_..._SomatoSync-Monorepo/2026-08-10__2522638b__项目检查请求.jsonl` | 62 |
| `sessions/c_..._SomatoSync-Monorepo/2026-08-19__2f32bcf9__检查算法和技术细节脱敏.jsonl` | 24 |
| `sessions/c_..._SomatoSync-Monorepo/2026-08-12__8bd284ca__ESP32-S3引脚分配图与菜单设计.jsonl` | 3 |
| `sessions/c_..._SomatoSync-Monorepo_apps_gateway_app/2026-08-12__679b8c9d__ESP32_崩溃原因分析.jsonl` | 1 |

除上表替换外，**脱敏包的其余字节与未脱敏原件逐字节完全相同**；
替换只改变字符串内容，未增删、未改写任何对话轮次或模型归属。

> **仓内索引的内网信息处理**：本目录随仓库提交的文件已把内网 IP 掩码为
> `192.168.0.***`，与 `docs/design/` 既有的脱敏口径保持一致；
> 未掩码的原始索引见上表外发材料。

## 六、完整性校验方法

```bash
# 1) 校验外发脱敏包本身
sha256sum VelaMotion_AI对话记录_外发脱敏版_20260919.zip
# 期望：08F0CAF737D20D30CD89BF1A1EBEE226F78A71FBB96CE7514B5B8A4CADBE4318

# 2) 解包后核对脱敏声明
unzip -q VelaMotion_AI对话记录_外发脱敏版_20260919.zip -d redacted
cat redacted/REDACTION_NOTES.json      # 替换规则（掩码 + 规则 SHA256）与逐文件替换次数
```

`redacted/raw-sha256.txt` 登记的是**未脱敏原件**每个文件的 SHA256：
若有需要，可据此核对「自留原件」与「外发脱敏版」的差异是否**仅限于**已披露的替换。
`REDACTION_NOTES.json` 中以**掩码 + 规则 SHA256** 的形式记录被替换的字面量，
以免声明文件本身再次泄漏密钥。

## 七、提取工具

`tools/` 下的两个脚本即为本次导出所用，可供第三方复现或审计：

- `extract-ai-sessions.py`：解析 VS Code Copilot Chat 的「快照 + 增量补丁」`.jsonl`，
  输出逐文件 SHA256、索引与机器可读清单。
- `export-mimocode.py`：以只读方式打开 MiMoCode 的 SQLite 库，
  按 `session` / `message` / `part` 三张表还原会话，并生成可读 Markdown 转写。

两个脚本均**不修改**任何源数据。
