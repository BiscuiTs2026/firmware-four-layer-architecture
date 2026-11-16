#!/usr/bin/env python3
"""check_layering.py — 四层架构启发式静态检查（v2）。

用法：
    python3 check_layering.py <工程根目录> [--max-per-file N]

检查项：
    1. 越层调用平台 API：app/、components/ 下出现 HAL / LL / SPL / RTOS 调用或寄存器访问 → ERROR
    2. 平台头泄漏：非 bsp 层的 .c 与 .h 出现芯片头或 RTOS 头 → ERROR
    3. 厂商类型泄漏：非 bsp 层头文件签名里出现厂商类型 → ERROR
    4. 反向依赖：下层 #include 上层的头文件 → ERROR
    5. 跨模块全局变量：非 bsp 层 .c 文件作用域的可写变量（含结构体/数组）→ ERROR
    6. extern 跨模块变量：非 bsp 层出现 extern 变量声明 → ERROR
    7. if 嵌套超过 3 层 → ERROR
    8. 未分层文件：四目录之外的 .c/.h 出现平台调用 → WARN
    9. APP 直调 BSP 或越权调用 OSAL → ERROR

豁免：在目标行加 `// arch-allow` 即可跳过该行检查。

退出码：0 = 无 ERROR；1 = 存在 ERROR；2 = 用法或路径错误。
"""
from __future__ import annotations

import argparse
import os
import re
import sys

LAYER_DEPTH = {"bsp": 0, "osal": 1, "components": 2, "app": 3}
LAYERS = set(LAYER_DEPTH)
UPPER_LAYERS = {"app", "components"}
NON_BSP_LAYERS = {"app", "components", "osal"}
APP_OSAL_ALLOWED = ("osal_sleep", "osal_tick_ms", "osal_now_ms", "osal_delay_ms")

PLATFORM_HEADER = re.compile(
    r'#\s*include\s*[<"]('
    r'stm32\w*_hal\.h|stm32\w+xx\.h|cmsis_os2?\.h|FreeRTOSConfig\.h|FreeRTOS\.h'
    r'|task\.h|queue\.h|semphr\.h|timers\.h|event_groups\.h|stream_buffer\.h|portmacro\.h'
    r'|rtthread\.h|gd32\w*\.h|nrfx?_\w*\.h|ti_msp_dl_config\.h|msp430\w*\.h'
    r')'
)

VENDOR_TYPE = re.compile(
    r"\b(SPI_HandleTypeDef|GPIO_TypeDef|TIM_HandleTypeDef|UART_HandleTypeDef|ADC_HandleTypeDef"
    r"|I2C_HandleTypeDef|DMA_HandleTypeDef|CAN_HandleTypeDef|RTC_HandleTypeDef"
    r"|HAL_\w+_TypeDef|I2C_TypeDef|USART_TypeDef|SPI_TypeDef|TIM_TypeDef|ADC_TypeDef"
    r"|nrfx_\w+_t|dl_\w+)\b"
)

HAL_CALL = re.compile(r"\b(?:__)?HAL_[A-Za-z0-9_]+\s*\(")
LL_CALL = re.compile(r"\b(?:__)?LL_[A-Za-z0-9_]+\s*\(")
SPL_CALL = re.compile(
    r"\b(?:gpio|rcu|usart|spi|i2c|adc|tim|dma|exti|crc|rtc|pmu|fwdgt|wwdgt|bkp|can|i2s|sdio|usbd)"
    r"_[a-z][a-z0-9_]*\s*\("
)
RTOS_CALL = re.compile(
    r"\b(vTaskDelay|vTaskDelayUntil|vTaskSuspend|vTaskStartScheduler|osDelay|osDelayUntil"
    r"|xSemaphoreTake|xSemaphoreGive|xQueueReceive|xQueueSend|xQueueReceiveFromISR"
    r"|xQueueSendFromISR|taskENTER_CRITICAL|taskEXIT_CRITICAL|portENTER_CRITICAL"
    r"|portEXIT_CRITICAL|ulTaskNotifyTake)\s*\("
)

REG_ACCESS = re.compile(
    r"->\s*(ODR|IDR|MODER|OTYPER|OSPEEDR|PUPDR|AFR[HL]?|BSRR|BRR|CR[HL]?|SR|DR|EGR"
    r"|CCR|ARR|PSC|CNT|DIER|SMCR|CFGR|PLLCFGR|BDTR|RCR)\d?\b"
)

SCALAR = (
    r"(?:unsigned\s+|signed\s+|const\s+|volatile\s+)*"
    r"(?:u?int(?:8|16|32|64)_t|char|short|int|long|float|double|bool|size_t"
    r"|[A-Za-z_]\w*_t|[A-Za-z_]\w*TypeDef)"
)
GLOBAL_VAR = re.compile(
    r"^(?:extern\s+)?(?:static\s+)?"
    r"(" + SCALAR + r")\s+\**\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:=[^;]*)?;\s*$"
)
EXTERN_VAR = re.compile(r"^extern\s+(?!.*\()\w.*;\s*$")

BSP_PREFIX_CALL = re.compile(r"\b(bsp|osal)_[a-z][a-z0-9_]*\s*\(")

SKIP_DIRS = {
    ".git", "build", "Debug", "Release", "Objects", "Listings", ".vscode",
    "node_modules", "Drivers", "Middlewares", "CMSIS", "tests",
}


def _mask_one_line(line: str):
    """屏蔽单行里的块注释、行注释与字符串内容。返回 (结果, 是否进入块注释)。"""
    out = []
    i = 0
    n = len(line)
    while i < n:
        if line.startswith("/*", i):
            j = line.find("*/", i + 2)
            if j >= 0:
                out.append(" " * (j + 2 - i))
                i = j + 2
            else:
                out.append(" " * (n - i))
                return "".join(out), True
        elif line.startswith("//", i):
            out.append(" " * (n - i))
            i = n
        elif line[i] in ('"', "'"):
            quote = line[i]
            out.append(quote)
            j = i + 1
            while j < n and line[j] != quote:
                out.append(" ")
                j += 1
            if j < n:
                out.append(quote)
                j += 1
            i = j
        else:
            out.append(line[i])
            i += 1
    return "".join(out), False


def mask_comments(text: str) -> str:
    """把注释与字符串内容替换为空格，保留行数。`#include` 行保留原始引号内容。"""
    out = []
    in_block = False
    for line in text.splitlines():
        if in_block:
            end = line.find("*/")
            if end >= 0:
                in_block = False
                out.append(" " * (end + 2) + _mask_one_line(line[end + 2 :])[0])
            else:
                out.append(" " * len(line))
            continue
        if line.lstrip().startswith("#include"):
            cut = line.find("//")
            out.append(line[:cut] if cut >= 0 else line)
            continue
        masked, in_block = _mask_one_line(line)
        out.append(masked)
    return "\n".join(out)


def layer_of(path: str, root: str) -> str:
    parts = os.path.relpath(path, root).replace("\\", "/").split("/")[:-1]
    for part in parts:
        if part in LAYERS:
            return part
    return ""


def iter_c_files(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if fn.endswith((".c", ".h")):
                yield os.path.join(dirpath, fn)


def brace_kind(code: str, idx: int) -> str:
    """返回 `{` 对应的结构类型：if / else_if / other。"""
    j = idx - 1
    while j >= 0 and code[j] in " \t\r\n":
        j -= 1
    if j < 0 or code[j] != ")":
        return "other"
    depth = 0
    k = j
    while k >= 0:
        if code[k] == ")":
            depth += 1
        elif code[k] == "(":
            depth -= 1
            if depth == 0:
                break
        k -= 1
    if k <= 0:
        return "other"
    m = k - 1
    while m >= 0 and code[m] in " \t\r\n":
        m -= 1
    end = m
    while m >= 0 and (code[m].isalnum() or code[m] == "_"):
        m -= 1
    if code[m + 1 : end + 1] != "if":
        return "other"
    m2 = m
    while m2 >= 0 and code[m2] in " \t\r\n":
        m2 -= 1
    end2 = m2
    while m2 >= 0 and (code[m2].isalnum() or code[m2] == "_"):
        m2 -= 1
    return "else_if" if code[m2 + 1 : end2 + 1] == "else" else "if"


def check_nesting(code: str):
    issues = []
    stack = []
    for i, ch in enumerate(code):
        if ch == "{":
            kind = brace_kind(code, i)
            if kind == "if":
                if_count = sum(1 for s in stack if s == "if")
                if if_count >= 3:
                    line_no = code.count("\n", 0, i) + 1
                    issues.append((line_no, f"if 嵌套 {if_count + 1} 层（上限 3）"))
                stack.append("if")
            else:
                stack.append("other")
        elif ch == "}":
            if stack:
                stack.pop()
    return issues


def find_global_vars(masked: str):
    """文件作用域（花括号深度 0）的可写变量声明。"""
    issues = []
    depth = 0
    for no, line in enumerate(masked.splitlines(), 1):
        code = line.strip()
        opens, closes = line.count("{"), line.count("}")
        if depth == 0 and code and code.endswith(";") and not code.startswith("#"):
            if (
                "(" in code
                or ")" in code
                or code.startswith("extern")
                or code.startswith("static")
                or code.startswith("const")
            ):
                depth += opens - closes
                continue
            m = GLOBAL_VAR.match(code)
            if m:
                issues.append((no, f"跨模块全局变量：{m.group(2)}（改走消息总线或改 static）"))
        depth += opens - closes
    return issues


def find_extern_vars(masked: str):
    issues = []
    depth = 0
    for no, line in enumerate(masked.splitlines(), 1):
        code = line.strip()
        if depth == 0 and code and EXTERN_VAR.match(code):
            issues.append((no, f"extern 跨模块变量：{code[:40]}"))
        depth += line.count("{") - line.count("}")
    return issues


def build_header_index(root: str):
    """头文件名 → 所在层。"""
    index = {}
    for path in iter_c_files(root):
        if not path.endswith(".h"):
            continue
        layer = layer_of(path, root)
        if not layer:
            continue
        index.setdefault(os.path.basename(path), layer)
    return index


def main():
    parser = argparse.ArgumentParser(description="四层架构启发式静态检查")
    parser.add_argument("root", help="工程根目录")
    parser.add_argument("--max-per-file", type=int, default=5, help="未分层文件每文件最多提示条数")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"路径不存在：{root}", file=sys.stderr)
        return 2

    header_index = build_header_index(root)
    errors, warns = [], []
    unlayered_hits = 0
    unlayered_files = 0

    for path in sorted(iter_c_files(root)):
        layer = layer_of(path, root)
        rel = os.path.relpath(path, root).replace("\\", "/")
        with open(path, encoding="utf-8", errors="ignore") as fh:
            raw = fh.read()
        masked = mask_comments(raw)
        lines = masked.splitlines()
        raw_lines = raw.splitlines()

        local_hits = 0
        for no, line in enumerate(lines, 1):
            if no <= len(raw_lines) and "arch-allow" in raw_lines[no - 1]:
                continue
            code = line

            if layer == "":
                hits = []
                for pat, label in (
                    (HAL_CALL, "HAL 调用"),
                    (LL_CALL, "LL 调用"),
                    (SPL_CALL, "厂商库调用"),
                    (RTOS_CALL, "RTOS 调用"),
                ):
                    m = pat.search(code)
                    if m:
                        hits.append(f"{label}：{m.group(0)}")
                        break
                if REG_ACCESS.search(code):
                    hits.append("寄存器访问")
                if hits and local_hits < args.max_per_file:
                    warns.append((rel, no, "未分层文件 " + "；".join(hits)))
                    local_hits += 1
                    unlayered_hits += 1
                continue

            if layer in NON_BSP_LAYERS:
                if layer != "osal" or path.endswith(".h"):
                    m = PLATFORM_HEADER.search(code)
                    if m:
                        errors.append((rel, no, f"平台头泄漏：{m.group(1)}"))

            if layer in UPPER_LAYERS:
                for pat, label in (
                    (HAL_CALL, "越层调用 HAL"),
                    (LL_CALL, "越层调用 LL"),
                    (SPL_CALL, "越层调用厂商库"),
                    (RTOS_CALL, "越层调用 RTOS API"),
                ):
                    m = pat.search(code)
                    if m:
                        errors.append((rel, no, f"{label}：{m.group(0)}"))
                if REG_ACCESS.search(code):
                    errors.append((rel, no, "越层访问寄存器"))

            if layer == "app":
                for m in BSP_PREFIX_CALL.finditer(code):
                    name = m.group(0).rstrip("(").strip()
                    if name.startswith("bsp_"):
                        errors.append((rel, no, f"APP 直调 BSP：{name}（应经组件层）"))
                    elif not name.startswith(APP_OSAL_ALLOWED):
                        errors.append((rel, no, f"APP 越权调用 OSAL：{name}（仅允许时间原语）"))

            if layer in NON_BSP_LAYERS and path.endswith(".h"):
                m = VENDOR_TYPE.search(code)
                if m:
                    errors.append((rel, no, f"厂商类型泄漏：{m.group(1)}"))

            for m in re.finditer(r'#\s*include\s*[<"]?([^>"]+)[>"]', code):
                target = m.group(1).replace("\\", "/").split("/")[-1]
                tgt_layer = header_index.get(target)
                if tgt_layer and LAYER_DEPTH.get(tgt_layer, -1) > LAYER_DEPTH[layer]:
                    errors.append((rel, no, f"反向依赖：{layer}/ 包含 {tgt_layer}/ 的 {target}"))
                    break

        if layer == "":
            if local_hits:
                unlayered_files += 1
            continue

        if path.endswith(".c") and layer in NON_BSP_LAYERS:
            for no, msg in find_global_vars(masked):
                errors.append((rel, no, msg))
            for no, msg in find_extern_vars(masked):
                errors.append((rel, no, msg))

        if layer != "bsp":
            for no, msg in check_nesting(masked):
                errors.append((rel, no, msg))

    for rel, no, msg in errors:
        print(f"[ERROR] {rel}:{no} {msg}")
    for rel, no, msg in warns:
        print(f"[WARN ] {rel}:{no} {msg}")

    if unlayered_files:
        print(
            f"[NOTE ] 另有 {unlayered_files} 个未分层文件、{unlayered_hits} 处平台调用未计入分层检查；"
            "把它们放进 app/ components/ osal/ bsp/ 才能纳入约束"
        )

    print(f"\n合计 {len(errors)} ERROR / {len(warns)} WARN（扫描目录：{root}）")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
