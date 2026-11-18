#!/usr/bin/env python3
"""tests/run_tests.py — 回归测试：期望行必须全部出现，且实际条数不能超出。

用法：python3 tests/run_tests.py
退出码：0 全部通过；1 有失败。
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CHECKER = os.path.join(ROOT, "skill", "scripts", "check_layering.py")
CASES = [
    ("tests/probe", os.path.join(ROOT, "tests", "probe", "expected.txt")),
    ("tests/probe-gaps", os.path.join(ROOT, "tests", "probe-gaps", "expected.txt")),
    ("tests/probe-flat", os.path.join(ROOT, "tests", "probe-flat", "expected.txt")),
    ("examples/bad", os.path.join(ROOT, "examples", "bad", "expected.txt")),
    ("tests/probe-issue2", os.path.join(ROOT, "tests", "probe-issue2", "expected.txt")),
    ("tests/probe-issue3", os.path.join(ROOT, "tests", "probe-issue3", "expected.txt")),
    ("tests/probe-issue4", os.path.join(ROOT, "tests", "probe-issue4", "expected.txt")),
    ("tests/probe-call-fp", os.path.join(ROOT, "tests", "probe-call-fp", "expected.txt")),
    ("tests/probe-hook-call", os.path.join(ROOT, "tests", "probe-hook-call", "expected.txt")),
    ("tests/probe-two-layer", os.path.join(ROOT, "tests", "probe-two-layer", "expected.txt")),
    ("tests/probe-layers-drift", os.path.join(ROOT, "tests", "probe-layers-drift", "expected.txt")),
]

# 示例工程必须零 ERROR、零 WARN
CLEAN = ["examples/good-hal", "examples/good-baremetal", "examples/flat", "examples/two-layer"]


def load_expected(path):
    expected = []
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if line.startswith(("[ERROR]", "[WARN ]")):
                expected.append(line)
    return expected


def run_case(rel_root, expected_path):
    target = os.path.join(ROOT, rel_root)
    proc = subprocess.run(
        [sys.executable, CHECKER, target],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    actual = [line.strip() for line in proc.stdout.splitlines() if line.startswith(("[ERROR]", "[WARN ]"))]
    expected = load_expected(expected_path)

    problems = []
    for line in expected:
        if not any(line in got for got in actual):
            problems.append(f"缺少期望项：{line}")
    if len(actual) != len(expected):
        problems.append(f"条数不符：实际 {len(actual)} 条，期望 {len(expected)} 条")
        for got in actual:
            if not any(exp in got for exp in expected):
                problems.append(f"多出未预期项：{got}")

    status = "PASS" if not problems else "FAIL"
    print(f"[{status}] {rel_root}：{len(actual)} 条发现（期望 {len(expected)} 条）")
    for item in problems:
        print(f"       {item}")
    return not problems


def run_clean(rel_root):
    target = os.path.join(ROOT, rel_root)
    proc = subprocess.run(
        [sys.executable, CHECKER, target],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    findings = [line.strip() for line in proc.stdout.splitlines() if line.startswith(("[ERROR]", "[WARN ]"))]
    status = "PASS" if not findings and proc.returncode == 0 else "FAIL"
    print(f"[{status}] {rel_root}（示例工程）：{len(findings)} 条发现")
    for item in findings:
        print(f"       {item}")
    return not findings and proc.returncode == 0


def run_unlayered_guard():
    """没有任何分层信息时必须失败，不能以退出码 0 静默通过。"""
    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "app.c"), "w", encoding="utf-8") as fh:
            fh.write('#include "stm32f1xx_hal.h"\nvoid f(void) { HAL_Init(); }\n')
        proc = subprocess.run(
            [sys.executable, CHECKER, tmp],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    ok = proc.returncode == 1 and "未发现任何分层目录或角色清单" in proc.stdout
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] 无角色清单守卫：退出码 {proc.returncode}（期望 1）")
    return ok


def run_vendored_copy():
    """把检查器单文件拷到工程外（模拟使用方 tools/）后，结果必须与仓库内一致。"""
    target = os.path.join(ROOT, "tests", "probe-flat")
    with tempfile.TemporaryDirectory() as tmp:
        vendored = os.path.join(tmp, "tools", "check_layering.py")
        os.makedirs(os.path.dirname(vendored))
        shutil.copyfile(CHECKER, vendored)
        proc = subprocess.run(
            [sys.executable, vendored, target],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        ver = subprocess.run(
            [sys.executable, vendored, "--version"],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    findings = [line for line in proc.stdout.splitlines() if line.startswith(("[ERROR]", "[WARN ]"))]
    ok = (
        len(findings) == len(load_expected(os.path.join(target, "expected.txt")))
        and ver.returncode == 0
        and "checker-version:" in ver.stdout
    )
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] 单文件拷出运行：{len(findings)} 条发现，--version 退出码 {ver.returncode}")
    return ok


def run_version_lock():
    """--version 的输出必须等于文件里声明的版本，锁文件一致返回 0、不一致返回 1。"""
    with open(CHECKER, encoding="utf-8") as fh:
        source = fh.read()
    match = re.search(r'^CHECKER_VERSION\s*=\s*"([^"]+)"', source, re.MULTILINE)
    declared = match.group(1) if match else None
    proc = subprocess.run(
        [sys.executable, CHECKER, "--version"],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    version = None
    for line in proc.stdout.splitlines():
        if line.startswith("checker-version:"):
            version = line.split(":", 1)[1].strip()
    if not declared or version != declared:
        print(f"[FAIL] --version 输出 {version!r} 与文件声明 {declared!r} 不一致")
        return False
    with tempfile.TemporaryDirectory() as tmp:
        good = os.path.join(tmp, "good.lock")
        bad = os.path.join(tmp, "bad.lock")
        body = (
            "checker-version = {v}\n"
            "source-repo = https://github.com/BiscuiTs2026/firmware-four-layer-architecture\n"
            "source-commit = self\n"
            "synced-at = 2026-11-18\n"
            "local-patches = none\n"
        )
        with open(good, "w", encoding="utf-8-sig") as fh:  # 带 BOM，模拟 Windows 编辑器
            fh.write(body.format(v=version))
        with open(bad, "w", encoding="utf-8") as fh:
            fh.write(body.format(v="0.0.1"))
        ok_proc = subprocess.run([sys.executable, CHECKER, "--version", "--lock", good],
                                 capture_output=True, text=True)
        bad_proc = subprocess.run([sys.executable, CHECKER, "--version", "--lock", bad],
                                  capture_output=True, text=True)
    ok = ok_proc.returncode == 0 and bad_proc.returncode == 1
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] --version={version} 与声明一致；锁文件：一致 {ok_proc.returncode}"
          f"（期望 0），不一致 {bad_proc.returncode}（期望 1）")
    return ok


def main():
    ok = True
    for rel_root, expected_path in CASES:
        ok = run_case(rel_root, expected_path) and ok
    for rel_root in CLEAN:
        ok = run_clean(rel_root) and ok
    ok = run_unlayered_guard() and ok
    ok = run_vendored_copy() and ok
    ok = run_version_lock() and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
