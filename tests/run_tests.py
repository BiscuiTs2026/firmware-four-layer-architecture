#!/usr/bin/env python3
"""tests/run_tests.py — 回归测试：期望行必须全部出现，且实际条数不能超出。

用法：python3 tests/run_tests.py
退出码：0 全部通过；1 有失败。
"""
from __future__ import annotations

import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CHECKER = os.path.join(ROOT, "skill", "scripts", "check_layering.py")
CASES = [
    ("tests/probe", os.path.join(ROOT, "tests", "probe", "expected.txt")),
    ("tests/probe-flat", os.path.join(ROOT, "tests", "probe-flat", "expected.txt")),
    ("examples/bad", os.path.join(ROOT, "examples", "bad", "expected.txt")),
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


def main():
    ok = True
    for rel_root, expected_path in CASES:
        ok = run_case(rel_root, expected_path) and ok
    for rel_root in CLEAN:
        ok = run_clean(rel_root) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
