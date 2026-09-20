#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
从 VS Code 的 workspaceStorage 里提取 SomatoSync / openvela 相关的 AI 对话记录。

设计原则
--------
1. **不改动原始文件**：逐字节复制（raw copy），不重排、不截断、不改 schema。
2. 只提取工作目录名匹配 somato / vela 的工作区（即本项目）。
3. INDEX.md / manifest.json 是**派生产物**，不是日志本体。

关于 VS Code 会话文件格式（重要）
--------------------------------
每个 `.jsonl` 是「快照 + 增量补丁」日志：
  - 首行  {"kind":0, "v": {整个会话状态}}          ← 快照，可能很旧甚至 requests 为空
  - 后续行 {"kind":1, "k": [路径...], "v": 值}     ← 补丁，路径里的数字是数组下标

补丁有两种形态，**两种都必须处理**：
  A. 整体追加一条请求： k = ["requests", 53]            ← 长度只有 2
  B. 修改请求的某个字段：k = ["requests", 53, "result"]  ← 更长

只处理 B 会漏掉 A 里整条请求的 modelId / 时间戳 / 提问，导致轮数与模型统计偏小。
本脚本对 A 做整体吸收，对 B 按**路径末段字段名**处理（不限层级），
并额外用有界递归兜底抓取嵌套结构里的 modelId / resolvedModel。
"""

import hashlib
import json
import os
import pathlib
import re
import shutil
import sys
from datetime import datetime, timezone
from urllib.parse import unquote, urlparse

APPDATA = pathlib.Path(os.environ.get("APPDATA", ""))
STORAGE = APPDATA / "Code" / "User" / "workspaceStorage"
OUT = pathlib.Path(r"C:\Users\ZYS20\Desktop\openvlea\ai-coding-sessions")
MATCH = re.compile(r"somato|openvela|vela", re.IGNORECASE)

MODEL_KEYS = ("modelId", "resolvedModel")


def safe(name: str, maxlen: int = 60) -> str:
    name = re.sub(r"[\\/:*?\"<>|\r\n\t]+", "_", name or "").strip(" ._")
    name = re.sub(r"\s+", "_", name)
    return (name[:maxlen] or "untitled")


def folder_of(ws_dir: pathlib.Path) -> str:
    wj = ws_dir / "workspace.json"
    if not wj.exists():
        return ""
    try:
        raw = json.loads(wj.read_text(encoding="utf-8")).get("folder", "") or ""
    except Exception:
        return ""
    if raw.startswith("file://"):
        return unquote(urlparse(raw).path).lstrip("/").replace("\\", "/")
    return raw


def scan_models(v, out: set, depth: int = 0) -> None:
    """有界递归，抓嵌套结构里的 modelId / resolvedModel。"""
    if depth > 6:
        return
    if isinstance(v, dict):
        for k, val in v.items():
            if k in MODEL_KEYS and isinstance(val, str):
                out.add(val)
            elif isinstance(val, (dict, list)):
                scan_models(val, out, depth + 1)
    elif isinstance(v, list):
        for x in v[:50]:
            if isinstance(x, (dict, list)):
                scan_models(x, out, depth + 1)


def absorb_request(meta: dict, r, idx: int = -1) -> None:
    """把一条请求计入统计。"""
    if not isinstance(r, dict):
        return
    models = set()
    scan_models(r, models)
    m = r.get("modelId")
    if isinstance(m, str):
        models.add(m)
    for x in models:
        if x not in meta["models"]:
            meta["models"].append(x)

    meta["prompt_tokens"] += r.get("promptTokens") or 0
    meta["completion_tokens"] += r.get("completionTokens") or 0

    ts = r.get("timestamp")
    if isinstance(ts, int):
        meta["ts_min"] = ts if meta["ts_min"] is None else min(meta["ts_min"], ts)
        meta["ts_max"] = ts if meta["ts_max"] is None else max(meta["ts_max"], ts)

    t = ((r.get("message") or {}).get("text") or "").strip()
    if t:
        meta["prompts"].append((idx, re.sub(r"\s+", " ", t).strip()))


def read_meta(path: pathlib.Path) -> dict:
    meta = {
        "session_id": path.stem,
        "title": "",
        "created": None,
        "models": [],
        "session_models": [],
        "req_idx": set(),
        "prompt_tokens": 0,
        "completion_tokens": 0,
        "ts_min": None,
        "ts_max": None,
        "prompts": [],
        "lines": 0,
        "patches": 0,
        "snapshot_requests": 0,
        "whole_appends": 0,
    }

    with path.open(encoding="utf-8", errors="replace") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            meta["lines"] += 1
            try:
                o = json.loads(line)
            except Exception:
                continue

            kind = o.get("kind")

            if kind == 0:
                v = o.get("v") or {}
                meta["session_id"] = v.get("sessionId") or meta["session_id"]
                meta["created"] = v.get("creationDate") or meta["created"]
                if v.get("customTitle"):
                    meta["title"] = v["customTitle"]
                reqs = v.get("requests") or []
                meta["snapshot_requests"] = len(reqs)
                for i, r in enumerate(reqs):
                    meta["req_idx"].add(i)
                    absorb_request(meta, r, i)
                sm = ((v.get("inputState") or {}).get("selectedModel") or {}).get("identifier")
                if isinstance(sm, str) and sm not in meta["session_models"]:
                    meta["session_models"].append(sm)
                continue

            if kind != 1:
                continue

            meta["patches"] += 1
            k = o.get("k")
            v = o.get("v")
            if not isinstance(k, list) or not k:
                continue

            # ---- 会话级字段 ----
            if k == ["customTitle"] and isinstance(v, str):
                meta["title"] = v
                continue
            if k[-1] == "identifier" and isinstance(v, str) and "selectedModel" in k:
                if v not in meta["session_models"]:
                    meta["session_models"].append(v)
                continue
            if k[0] != "requests" or not isinstance(k[1], int):
                continue

            idx = k[1]
            meta["req_idx"].add(idx)

            # ---- 形态 A：整条请求追加 ----
            if len(k) == 2:
                meta["whole_appends"] += 1
                if isinstance(v, dict):
                    absorb_request(meta, v, idx)
                elif isinstance(v, list):
                    for x in v:
                        absorb_request(meta, x, idx)
                continue

            # ---- 形态 B：字段级补丁，按路径末段字段名处理 ----
            field = k[-1]
            if field in MODEL_KEYS and isinstance(v, str):
                if v not in meta["models"]:
                    meta["models"].append(v)
            elif field in ("promptTokens", "completionTokens") and isinstance(v, int):
                meta[{"promptTokens": "prompt_tokens",
                      "completionTokens": "completion_tokens"}[field]] += v
            elif field == "timestamp" and isinstance(v, int):
                meta["ts_min"] = v if meta["ts_min"] is None else min(meta["ts_min"], v)
                meta["ts_max"] = v if meta["ts_max"] is None else max(meta["ts_max"], v)
            elif field == "message" or (field == "text" and "message" in k):
                text = ""
                if isinstance(v, dict):
                    text = v.get("text") or ""
                elif isinstance(v, str):
                    text = v
                if text:
                    meta["prompts"].append((idx, re.sub(r"\s+", " ", text).strip()))
            else:
                # 兜底：值里可能藏着 modelId / resolvedModel
                found = set()
                scan_models(v, found)
                for x in found:
                    if x not in meta["models"]:
                        meta["models"].append(x)

    return meta


def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    if not STORAGE.exists():
        print(f"找不到 VS Code 存储目录：{STORAGE}")
        return 1

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "sessions").mkdir(exist_ok=True)

    entries, skipped = [], []

    for ws_dir in sorted(STORAGE.iterdir()):
        if not ws_dir.is_dir():
            continue
        folder = folder_of(ws_dir)
        if not MATCH.search(folder):
            continue
        cs = ws_dir / "chatSessions"
        if not cs.is_dir():
            continue
        ws_slug = safe(folder.replace("/", "_").replace(":", ""), 70)

        for f in sorted(cs.glob("*.jsonl")):
            if f.stat().st_size == 0:
                skipped.append((folder, f.name, "0 字节"))
                continue

            meta = read_meta(f)
            sid = meta["session_id"]
            title = meta["title"] or "(无标题)"
            created = meta["created"]
            dt = (datetime.fromtimestamp(created / 1000, tz=timezone.utc).astimezone()
                  if created else None)
            day = dt.strftime("%Y-%m-%d") if dt else "unknown-date"

            prompts = [t for _, t in sorted(meta["prompts"], key=lambda x: x[0])][:3]

            dest_dir = OUT / "sessions" / ws_slug
            dest_dir.mkdir(parents=True, exist_ok=True)
            dest = dest_dir / f"{day}__{sid[:8]}__{safe(title)}.jsonl"
            shutil.copy2(f, dest)          # 原样复制，不改内容

            entries.append({
                "workspace": folder,
                "title": title,
                "session_id": sid,
                "created": dt.isoformat() if dt else None,
                "requests": len(meta["req_idx"]),
                "models": meta["models"],
                "session_models": meta["session_models"],
                "prompt_tokens": meta["prompt_tokens"],
                "completion_tokens": meta["completion_tokens"],
                "log_lines": meta["lines"],
                "patch_lines": meta["patches"],
                "whole_appends": meta["whole_appends"],
                "source_file": str(f),
                "source_bytes": f.stat().st_size,
                "output_file": str(dest.relative_to(OUT)).replace("\\", "/"),
                "sha256": sha256(dest),
                "first_prompts": prompts,
            })

    entries.sort(key=lambda e: -e["source_bytes"])

    (OUT / "manifest.json").write_text(json.dumps({
        "generated": datetime.now().astimezone().isoformat(),
        "source_root": str(STORAGE),
        "note": "原始文件逐字节复制，未修改内容。轮数/模型/token 由快照与增量补丁（含整条追加）聚合得出。",
        "sessions": entries,
        "skipped": [{"workspace": w, "file": n, "reason": r} for w, n, r in skipped],
    }, ensure_ascii=False, indent=2), encoding="utf-8")

    def norm(models):
        """把 mimo/mimo-v2.5-pro 与 mimo-v2.5-pro 归并统计。"""
        base = {}
        for m in models:
            short = m.split("/")[-1]
            base[short] = base.get(short, 0) + 1
        return base

    tot_mb = sum(e["source_bytes"] for e in entries) / 1024 / 1024
    tot_req = sum(e["requests"] for e in entries)
    tot_in = sum(e["prompt_tokens"] for e in entries)
    tot_out = sum(e["completion_tokens"] for e in entries)

    # 两种口径必须分开统计：把二者取并集会高估
    #   models         = 每条请求记录里的 modelId，说明该模型**确实产出了回答**（权威）
    #   session_models = composer 的 inputState.selectedModel，只说明下拉框选了它；
    #                    有些会话（多为无标题的空会话）没有任何请求记录。
    per_model_req: dict[str, set] = {}
    per_model_sel: dict[str, set] = {}
    for e in entries:
        for m in set(e["models"]):
            per_model_req.setdefault(m.split("/")[-1], set()).add(e["session_id"])
        for m in set(e["session_models"]):
            per_model_sel.setdefault(m.split("/")[-1], set()).add(e["session_id"])

    L = [
        "# AI 对话记录索引（VS Code / GitHub Copilot Chat）",
        "",
        f"- 提取时间：{datetime.now().astimezone().strftime('%Y-%m-%d %H:%M')}",
        f"- 会话文件 **{len(entries)}** 个，合计 **{tot_mb:.1f} MB**（其中 {sum(1 for e in entries if e['requests'])} 个含有效对话）",
        f"- 对话轮数 **{tot_req}**",
        f"- 累计 token：输入 **{tot_in:,}**　输出 **{tot_out:,}**",
        f"- 来源：`{STORAGE}`",
        "- **逐字节原样复制，未修改任何内容**",
        "",
        "## 模型使用情况（按会话数，两种口径分列）",
        "",
        "| 模型 | 有请求级证据的会话（权威） | UI 中被选中的会话 |",
        "| --- | --- | --- |",
    ]
    all_models = set(per_model_req) | set(per_model_sel)
    for m in sorted(all_models,
                    key=lambda x: (-len(per_model_req.get(x, ())), -len(per_model_sel.get(x, ())))):
        L.append(f"| `{m}` | {len(per_model_req.get(m, ()))} | {len(per_model_sel.get(m, ()))} |")

    L += [
        "",
        "> `mimo-v2.5-pro` 由扩展 `sdmapvstool.xiaomimimo-for-copilot` 提供（小米 MiMo）。",
        "",
        "> **两列不可混用**：左列取**每条请求记录的 modelId**，表示该模型**确实产出了回答**；",
        "> 右列取 composer 的 `inputState.selectedModel`，只表示下拉框选中了它。",
        "> 右列大于左列的部分，是**没有任何请求记录的空会话**（多为无标题会话），",
        "> 不应据此宣称该模型被实际使用过。",
        "> `copilot/auto` 表示由 Copilot 自动选择模型、未固定。",
        "",
        "> 说明：VS Code 会话文件是「快照 + 增量补丁」格式，补丁既可能整条追加请求",
        "> （`k=[\"requests\",N]`）、也可能改单字段（`k=[\"requests\",N,\"modelId\"]`）。",
        "> 本索引两种形态都统计，并对嵌套结构做有界递归兜底。",
        "",
        "| # | 工作区 | 会话标题 | 日期 | 轮数 | 模型 | 输入 tok | 输出 tok | 大小 | SHA256 |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for i, e in enumerate(entries, 1):
        if e["models"]:
            ms = ", ".join(e["models"])
        elif e["session_models"]:
            ms = ", ".join(e["session_models"]) + " *"
        else:
            ms = "-"
        L.append("| {} | `{}` | {} | {} | {} | {} | {} | {} | {:.2f} MB | `{}` |".format(
            i, e["workspace"], e["title"].replace("|", "/"),
            (e["created"] or "")[:10], e["requests"], ms,
            f'{e["prompt_tokens"]:,}', f'{e["completion_tokens"]:,}',
            e["source_bytes"] / 1024 / 1024, e["sha256"][:16]))

    if skipped:
        L += ["", "## 跳过（0 字节）", ""] + [f"- `{w}` / `{n}`" for w, n, r in skipped]
    (OUT / "INDEX.md").write_text("\n".join(L) + "\n", encoding="utf-8")

    print(f"sessions={len(entries)} size={tot_mb:.1f}MB turns={tot_req} "
          f"in={tot_in} out={tot_out}")
    print("models(sessions): " + ", ".join(
        f"{m}=req{len(per_model_req.get(m, ()))}/sel{len(per_model_sel.get(m, ()))}"
        for m in sorted(all_models,
                        key=lambda x: (-len(per_model_req.get(x, ())),
                                       -len(per_model_sel.get(x, ()))))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
