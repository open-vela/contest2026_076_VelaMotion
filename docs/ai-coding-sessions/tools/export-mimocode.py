#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
从构建机（一台内网 Ubuntu 主机）的 MiMoCode SQLite 数据库中导出 AI 对话记录。

主机名与内网 IP 通过环境变量 `MIMOCODE_SRC_HOST` 传入，仓库内不写入内网信息。

设计原则
--------
1. **不改动原始数据**：`raw/mimocode.db` 是服务器上 `sqlite3.backup()` 做出来的一致性快照，
   逐字节保存；下面导出的 JSONL 是**按会话分组的原始行**（`data` 字段原样保留），
   只做分组与排序，不做内容改写。
2. 另生成 `transcripts/*.md` 供人阅读 —— 那是**派生产物**，可能省略工具调用的细节，
   以 `sessions/*.jsonl` 与 `raw/mimocode.db` 为准。

输入：ai-coding-sessions/mimocode/raw/mimocode.db
输出：ai-coding-sessions/mimocode/{INDEX.md, manifest.json, README.md,
      sessions/*.jsonl, transcripts/*.md}
"""

import json
import os
import pathlib
import re
import sqlite3
import sys
from datetime import datetime, timezone

# 导出根目录：默认与脚本同级的 ai-coding-sessions/mimocode，可用环境变量覆盖
ROOT = pathlib.Path(
    os.environ.get(
        "MIMOCODE_EXPORT_ROOT",
        pathlib.Path(__file__).resolve().parent / "ai-coding-sessions" / "mimocode",
    )
)
DB = ROOT / "raw" / "mimocode.db"
# 来源标注：为避免公开仓库中出现内网信息，默认使用脱敏占位符
SRC_HOST = os.environ.get("MIMOCODE_SRC_HOST", "<构建机主机名/内网 IP 已脱敏>")
SRC_DB = os.environ.get("MIMOCODE_SRC_DB", "~/.local/share/mimocode/mimocode.db")


def safe(name: str, maxlen: int = 60) -> str:
    name = re.sub(r"[\\/:*?\"<>|\r\n\t]+", "_", name or "").strip(" ._")
    name = re.sub(r"\s+", "_", name)
    return (name[:maxlen] or "untitled")


def load(v):
    if v is None:
        return {}
    if isinstance(v, (bytes, bytearray)):
        v = v.decode("utf-8", "replace")
    if isinstance(v, str):
        try:
            return json.loads(v)
        except Exception:
            return {"_raw": v}
    return v


def main() -> int:
    if not DB.exists():
        print(f"找不到数据库：{DB}")
        return 1

    (ROOT / "sessions").mkdir(parents=True, exist_ok=True)
    (ROOT / "transcripts").mkdir(parents=True, exist_ok=True)

    con = sqlite3.connect(f"file:{DB}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    cur = con.cursor()

    sessions = cur.execute("SELECT * FROM session ORDER BY time_created").fetchall()

    entries = []
    grand_msgs = grand_parts = 0
    grand_in = grand_out = 0
    all_models = {}

    for s in sessions:
        sid = s["id"]
        title = s["title"] or "(无标题)"
        directory = s["directory"] or ""
        created = s["time_created"]
        dt = (datetime.fromtimestamp(created / 1000, tz=timezone.utc).astimezone()
              if created else None)
        day = dt.strftime("%Y-%m-%d") if dt else "unknown-date"

        msgs = cur.execute(
            "SELECT * FROM message WHERE session_id=? ORDER BY time_created", (sid,)).fetchall()
        parts = cur.execute(
            "SELECT * FROM part WHERE session_id=? ORDER BY time_created", (sid,)).fetchall()

        parts_by_msg = {}
        for p in parts:
            parts_by_msg.setdefault(p["message_id"], []).append(p)

        slug = f"{day}__{sid[4:12]}__{safe(title)}"

        # ---------- 忠实导出：按会话分组的原始行 ----------
        jl = ROOT / "sessions" / f"{slug}.jsonl"
        tin = tout = 0
        models = []
        with jl.open("w", encoding="utf-8") as fh:
            for m in msgs:
                md = load(m["data"])
                mrec = {
                    "table": "message",
                    "id": m["id"], "session_id": sid,
                    "time_created": m["time_created"], "time_updated": m["time_updated"],
                    "data": md,                     # 原样
                }
                fh.write(json.dumps(mrec, ensure_ascii=False) + "\n")

                mid = md.get("modelID") or (md.get("model") or {}).get("modelID")
                pid = md.get("providerID") or (md.get("model") or {}).get("providerID")
                if mid:
                    key = f"{pid}/{mid}" if pid else mid
                    if key not in models:
                        models.append(key)
                    all_models[key] = all_models.get(key, 0) + 1
                tk = md.get("tokens") or {}
                tin += tk.get("input") or 0
                tout += tk.get("output") or 0

                for p in parts_by_msg.get(m["id"], []):
                    prec = {
                        "table": "part",
                        "id": p["id"], "message_id": p["message_id"], "session_id": sid,
                        "time_created": p["time_created"],
                        "data": load(p["data"]),    # 原样
                    }
                    fh.write(json.dumps(prec, ensure_ascii=False) + "\n")

        # ---------- 可读转写稿（派生产物）----------
        lines = [
            f"# {title}",
            "",
            f"- 会话 ID：`{sid}`",
            f"- 工作目录：`{directory}`",
            f"- 时间：{dt.strftime('%Y-%m-%d %H:%M:%S') if dt else '-'}",
            f"- 模型：{', '.join('`'+m+'`' for m in models) or '-'}",
            f"- 消息 {len(msgs)} 条 / 部件 {len(parts)} 个；token 输入 {tin:,} 输出 {tout:,}",
            "",
            "> 本文件是可读转写稿（派生产物）。权威数据见同级 `sessions/*.jsonl` 与 `raw/mimocode.db`。",
            "",
            "---",
            "",
        ]
        for m in msgs:
            md = load(m["data"])
            role = md.get("role") or "?"
            t = md.get("time") or {}
            ts = t.get("created") or m["time_created"]
            when = (datetime.fromtimestamp(ts / 1000, tz=timezone.utc).astimezone()
                    .strftime("%m-%d %H:%M:%S") if ts else "")
            lines.append(f"## {'👤 用户' if role == 'user' else '🤖 助手'}　{when}")
            lines.append("")
            for p in parts_by_msg.get(m["id"], []):
                pd = load(p["data"])
                pt = pd.get("type")
                if pt == "text":
                    lines.append((pd.get("text") or "").strip())
                    lines.append("")
                elif pt == "reasoning":
                    r = (pd.get("text") or "").strip()
                    if r:
                        lines.append("<details><summary>思考过程</summary>")
                        lines.append("")
                        lines.append(r[:4000])
                        lines.append("")
                        lines.append("</details>")
                        lines.append("")
                elif pt == "tool":
                    st = pd.get("state") or {}
                    tool = pd.get("tool") or "?"
                    inp = st.get("input") or {}
                    lines.append(f"**🔧 工具调用 `{tool}`** — `{json.dumps(inp, ensure_ascii=False)[:300]}`")
                    lines.append("")
            lines.append("---")
            lines.append("")

        (ROOT / "transcripts" / f"{slug}.md").write_text("\n".join(lines), encoding="utf-8")

        grand_msgs += len(msgs)
        grand_parts += len(parts)
        grand_in += tin
        grand_out += tout

        entries.append({
            "session_id": sid, "title": title, "directory": directory,
            "created": dt.isoformat() if dt else None,
            "messages": len(msgs), "parts": len(parts),
            "models": models, "prompt_tokens": tin, "completion_tokens": tout,
            "session_jsonl": f"sessions/{slug}.jsonl",
            "transcript": f"transcripts/{slug}.md",
        })

    (ROOT / "manifest.json").write_text(json.dumps({
        "generated": datetime.now().astimezone().isoformat(),
        "source_host": SRC_HOST,
        "source_db": SRC_DB,
        "raw_snapshot": "raw/mimocode.db",
        "note": "sessions/*.jsonl 为按会话分组的原始数据库行，data 字段原样保留；"
                "transcripts/*.md 为可读派生产物。",
        "sessions": entries,
    }, ensure_ascii=False, indent=2), encoding="utf-8")

    L = [
        "# MiMoCode 会话索引（构建机）",
        "",
        f"- 导出时间：{datetime.now().astimezone().strftime('%Y-%m-%d %H:%M')}",
        f"- 来源：`{SRC_HOST}` 的 `{SRC_DB}`（一致性快照，见 `raw/mimocode.db`）",
        f"- 会话 **{len(entries)}** 个，消息 **{grand_msgs}** 条，部件 **{grand_parts}** 个",
        f"- 累计 token：输入 **{grand_in:,}**　输出 **{grand_out:,}**",
        "",
        "## 模型（来自每条消息的 `providerID` / `modelID`）",
        "",
        "| 模型 | 出现消息数 |",
        "| --- | --- |",    ]
    for m, c in sorted(all_models.items(), key=lambda x: -x[1]):
        L.append(f"| `{m}` | {c} |")
    L += [
        "",
        "> `xiaomi/mimo-v2.5-pro`、`mimo/mimo-auto` 均为**小米 MiMo**；",
        "> 另见 `raw/model.json` 中的 `{\"providerID\":\"xiaomi\",\"modelID\":\"mimo-v2.5-pro\"}`。",
        "",
        "| # | 会话标题 | 工作目录 | 日期 | 消息 | 部件 | 模型 | 输入 tok | 输出 tok |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for i, e in enumerate(entries, 1):
        L.append("| {} | {} | `{}` | {} | {} | {} | {} | {} | {} |".format(
            i, e["title"].replace("|", "/")[:56], e["directory"],
            (e["created"] or "")[:10], e["messages"], e["parts"],
            ", ".join(e["models"]) or "-",
            f'{e["prompt_tokens"]:,}', f'{e["completion_tokens"]:,}'))
    (ROOT / "INDEX.md").write_text("\n".join(L) + "\n", encoding="utf-8")

    print(f"sessions={len(entries)} messages={grand_msgs} parts={grand_parts}")
    print("models:", ", ".join(f"{m}={c}" for m, c in sorted(all_models.items(), key=lambda x: -x[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
