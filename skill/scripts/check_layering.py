#!/usr/bin/env python3
"""check_layering.py — 四层架构启发式静态检查。

用法：
    python3 check_layering.py <工程根目录>

检查项：
    1. 越层调用：app/、components/ 下出现 HAL_* 调用或寄存器访问   → ERROR
    2. 平台头泄漏：app/、components/、osal/ 的头文件 include 芯片或 RTOS 头 → ERROR
    3. 厂商类型泄漏：同上的头文件签名里出现厂商类型                → ERROR
    4. 跨模块全局变量：非 bsp 层 .c 文件作用域的可写变量            → WARN
    5. if-else 嵌套超过 3 层                                       → WARN

豁免：在目标行加 `// arch-allow` 即可跳过该行检查。

退出码：0 = 无 ERROR；1 = 存在 ERROR；2 = 用法或路径错误。
"""
from __future__ import annotations

import argparse
import os
import re
import sys

UPPER_LAYERS = {"app", "components"}            # 不允许碰 HAL / 寄存器
NON_BSP_LAYERS = {"app", "components", "osal"}   # 不允许泄漏平台头 / 厂商类型

PLATFORM_HEADER = re.compile(
    r'#\s*include\s*[<"](stm32\w*_hal\.h|stm32\w+xx\.h|FreeRTOS\.h|cmsis_os2?\.h|rtthread\.h|nrfx?\w*\.h)'
)
VENDOR_TYPE = re.compile(
    r"\b(SPI_HandleTypeDef|GPIO_TypeDef|TIM_HandleTypeDef|UART_HandleTypeDef|ADC_HandleTypeDef"
    r"|I2C_HandleTypeDef|DMA_HandleTypeDef|CAN_HandleTypeDef|RTC_HandleTypeDef"
    r"|HAL_\w+_TypeDef|nrfx_\w+_t|dl_\w+)\b"
)
HAL_CALL = re.compile(r"\bHAL_[A-Za-z0-9_]+\s*\(")
REG_ACCESS = re.compile(r"->\s*(ODR|IDR|CR[HL]?|MODER|PUPDR|AFR|BSRR|SR|DR|CCR)\b")
SCALAR_TYPES = r"(?:unsigned\s+|signed\s+|const\s+|volatile\s+)*(?:u?int(?:8|16|32|64)_t|char|short|int|long|float|double|bool|size_t)"
GLOBAL_VAR = re.compile(r"^" + SCALAR_TYPES + r"\s+\**\s*([A-Za-z_]\w*)\s*(?:=[^;]*)?;\s*$")

SKIP_DIRS = {".git", "build", "Debug", "Release", "Objects", "Listings", ".vscode", "node_modules"}


def strip_code(line: str) -> str:
    """去掉行注释与字符串内容，降低误报。"""
    line = re.sub(r"//.*$", "", line)
    line = re.sub(r'"[^"]*"', '""', line)
    return line


def layer_of(path: str, root: str) -> str:
    parts = os.path.relpath(path, root).replace("\\", "/").split("/")[:-1]
    for part in parts:
        if part in ("app", "components", "osal", "bsp"):
            return part
    return ""


def iter_c_files(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if fn.endswith((".c", ".h")):
                yield os.path.join(dirpath, fn)


def check_nesting(text: str):
    """按花括号层级统计 if 嵌套；else if 不计入新层。"""
    issues = []
    stack = []
    for i, ch in enumerate(text):
        if ch == "{":
            back = text[max(0, i - 90):i]
            is_else_if = re.search(r"else\s+if\s*\([^()]*\)\s*$", back) is not None
            is_if = is_else_if or re.search(r"\bif\s*\([^()]*\)\s*$", back) is not None
            if is_if and not is_else_if:
                if_count = sum(1 for s in stack if s == "if")
                if if_count >= 3:
                    line_no = text.count("\n", 0, i) + 1
                    issues.append((line_no, f"if-else 嵌套 {if_count + 1} 层（上限 3）"))
                stack.append("if")
            else:
                stack.append("other")
        elif ch == "}":
            if stack:
                stack.pop()
    return issues


def check_globals(lines):
    """文件作用域（花括号深度 0）的可写变量。"""
    issues = []
    depth = 0
    in_block_comment = False
    for no, raw in enumerate(lines, 1):
        line = raw
        if in_block_comment:
            if "*/" in line:
                line = line.split("*/", 1)[1]
                in_block_comment = False
            else:
                depth += raw.count("{") - raw.count("}")
                continue
        if "/*" in line and "*/" not in line:
            line = line.split("/*", 1)[0]
            in_block_comment = True
        code = strip_code(line).strip()
        if depth == 0 and code and code.endswith(";"):
            if not re.match(r"^(typedef|static|extern|struct|union|enum|#)", code):
                m = GLOBAL_VAR.match(code)
                if m:
                    issues.append((no, f"跨模块全局变量候选：{m.group(1)}（改走消息总线或加 static）"))
        depth += raw.count("{") - raw.count("}")
    return issues


def main():
    parser = argparse.ArgumentParser(description="四层架构启发式静态检查")
    parser.add_argument("root", help="工程根目录")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"路径不存在：{root}", file=sys.stderr)
        return 2

    errors, warns = [], []
    for path in sorted(iter_c_files(root)):
        layer = layer_of(path, root)
        with open(path, encoding="utf-8", errors="ignore") as fh:
            text = fh.read()
        rel = os.path.relpath(path, root)
        lines = text.splitlines()

        for no, raw in enumerate(lines, 1):
            if "arch-allow" in raw:
                continue
            code = strip_code(raw)
            if layer in UPPER_LAYERS:
                m = HAL_CALL.search(code)
                if m:
                    errors.append((rel, no, f"越层调用 HAL：{m.group(0)}"))
                if REG_ACCESS.search(code):
                    errors.append((rel, no, "越层访问寄存器"))
            if layer in NON_BSP_LAYERS and path.endswith(".h"):
                m = PLATFORM_HEADER.search(code)
                if m:
                    errors.append((rel, no, f"平台头泄漏：{m.group(1)}"))
                m = VENDOR_TYPE.search(code)
                if m:
                    errors.append((rel, no, f"厂商类型泄漏：{m.group(1)}"))

        if path.endswith(".c") and layer in NON_BSP_LAYERS:
            for no, msg in check_globals(lines):
                warns.append((rel, no, msg))
        if layer != "bsp":
            for no, msg in check_nesting(text):
                warns.append((rel, no, msg))

    for rel, no, msg in errors:
        print(f"[ERROR] {rel}:{no} {msg}")
    for rel, no, msg in warns:
        print(f"[WARN ] {rel}:{no} {msg}")

    print(f"\n合计 {len(errors)} ERROR / {len(warns)} WARN（扫描目录：{root}）")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
