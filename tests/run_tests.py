#!/usr/bin/env python3
"""tests/run_tests.py — 回归测试：期望行必须全部出现，且实际条数不能超出。

用法：python3 tests/run_tests.py
退出码：0 全部通过；1 有失败。
"""
from __future__ import annotations

import os
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
]

# 示例工程必须零 ERROR、零 WARN
CLEAN = ["examples/good-hal", "examples/good-baremetal", "examples/flat"]


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


def main():
    ok = True
    for rel_root, expected_path in CASES:
        ok = run_case(rel_root, expected_path) and ok
    for rel_root in CLEAN:
        ok = run_clean(rel_root) and ok
    ok = run_unlayered_guard() and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
