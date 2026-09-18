# logs/ — AI Coding 日志目录

存放开发中与 AI 工具的对话日志，和作品代码一并提交。

> 本目录当前只有这份说明；请把导出的真实日志按下面的结构放进来，
> 并删除任何示例目录。

## 目录结构

```text
logs/
└── <github_login>/              # 你的 GitHub 用户名，一人一目录
    ├── manifest.json            # 会话清单
    └── <date>/                  # 日期 YYYY-MM-DD
        └── <tool>__<sid>.jsonl  # 一个会话一个文件（工具名与 session id 用 __ 连接）
```

- `<tool>`：`claude-code` / `opencode` / `codex` / `kiro`
- 每个 `.jsonl` 每行一个事件，由组委会提供的日志归集工具导出，**只提交 JSONL 本身**。
- 每名队员一个以自己 GitHub 用户名命名的目录，日志按账号区分、合并时不冲突。

## 导出步骤（在 openvela 工作区内执行）

采集器只在识别到工作区根目录 `.repo/` 的目录树内激活。

```bash
# 1. 一次性安装（在专属仓目录内执行）
cd contest2026_076_VelaMotion
bash ../.claude/skills/contest-log-collector/onboarding/install.sh \
  --team-id contest2026_076_VelaMotion \
  --github-login <你的 GitHub 用户名>

# 2. 健康检查，出现 [FAIL] 按提示修复
bash ../.claude/skills/contest-log-collector/onboarding/verify-setup.sh

# 3. 补回安装 hook 之前的历史会话（可多次执行，不会重复）
contest-snapshot --backfill

# 4. 预览 → 确认导出（不加 --confirm 只预览，不写文件）
contest-snapshot --list
contest-snapshot --latest --confirm

# 5. 校验并提交
python3 ../.claude/skills/contest-log-collector/tools/validate-log.py logs/
git add logs/ && git commit -s -m "logs: capture session" && git push
```

## 注意事项

- 支持的工具只有 4 种：**Claude Code**（含 AIoT-IDE 内嵌）、**AIoT-IDE**、
  **OpenCode**、**Codex**。其他工具（ChatGPT/Cursor 等）与直接调用 API
  的对话**无法采集，不计入工时**。
- 采集器只写本机文件，**永远不会自动 `git push`**；上传与否完全由你决定。
- 不要修改 `.jsonl` 内容：`validate-log.py` 会检测序号断档与篡改。
  需要撤回某次对话时，在 commit 前直接删除对应文件即可。
- Windows 用户在 **Git Bash** 中执行上述命令（需 Python 3 + Git，无需 WSL）。

完整的字段定义与排错见
[《AI Coding 日志归集与提交手册》](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_coding_log_guide.md)。
