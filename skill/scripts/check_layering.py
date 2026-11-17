#!/usr/bin/env python3
"""check_layering.py — 四层架构启发式静态检查（v3）。

用法：
    python3 check_layering.py <工程根目录> [选项]

选项：
    --roles PATH        角色清单文件，默认在工程根目录找 .arch-roles / arch-roles.txt
    --glue-max-lines N  入口胶水文件行数上限，默认 200
    --max-per-file N    未分层文件每文件最多提示条数，默认 5
    --no-linker         跳过链接脚本固定地址段检查

两类工程形态都能扫：
    目录模式：路径里含 bsp/ osal/ components/ app/ 即按目录判层，无需任何配置。
    清单模式：工程是扁平 src/ 时，用角色清单把文件映射到层，例如
              src/i2c_soft.* = bsp
              src/telemetry.* = components
              src/main*.c   = glue

角色：
    bsp / osal / components / app —— 四层。
    common —— 纯类型与常量（err_t、公共枚举），任何层可包含它，它不参与依赖排序。
    glue —— 入口胶水（main.c、startup_*、*_irq.c 等），允许碰寄存器与 HAL，
            但只允许做初始化与中断分发，行数超限即判为"业务逻辑上移失败"。

检查项：
     1. 越层调用平台 API（app/ components/ 里出现 HAL/LL/SPL/RTOS 或寄存器）→ ERROR
     2. 平台头泄漏（非 bsp 层的 .c/.h 出现芯片头或 RTOS 头）→ ERROR
     3. 厂商类型泄漏（非 bsp 层头文件签名里出现厂商类型）→ ERROR
     4. 反向依赖（下层 #include 上层头文件）→ ERROR
     5. 跨模块全局变量（非 bsp 层 .c 文件作用域可写变量）→ ERROR
     6. extern 跨模块变量（非 bsp 层出现 extern 变量声明）→ ERROR
     7. 决策型 if 嵌套超过 3 层 → ERROR（单分支守卫不计，避免字模/像素循环误报）
     8. 未分层文件的平台调用 → WARN
     9. APP 直调 BSP 或越权调用 OSAL → ERROR
    10. 入口胶水层超过 --glue-max-lines → WARN
    11. ISR 内调用阻塞/格式化/动态内存函数 → ERROR
    12. 阻塞延时：bsp 之外的 HAL_Delay → ERROR；app/组件层裸 delay_ms 等 → WARN
    13. 固定地址段冲突（.ld 里两个段同址 / 未 4 字节对齐）→ ERROR；
        C 里 section 属性未在 .ld 定义且未被通配覆盖 → ERROR；未标 (NOLOAD) → WARN

豁免：
    - 行尾 `// arch-allow` 跳过该行全部检查；
    - 行尾 `// arch-allow:snapshot` 额外允许该行的全局变量声明（固定地址快照块）；
    - `SystemCoreClock` 属 CMSIS 必需符号，默认豁免。

退出码：0 = 无 ERROR；1 = 存在 ERROR；2 = 用法或路径错误。
"""
from __future__ import annotations

import argparse
import fnmatch
import os
import re
import sys

ROLES = ("bsp", "osal", "components", "app", "glue", "common")
LAYER_DEPTH = {"common": -1, "bsp": 0, "osal": 1, "components": 2, "app": 3}
LAYERS = set(LAYER_DEPTH)
UPPER_LAYERS = {"app", "components"}
NON_BSP_LAYERS = {"app", "components", "osal", "common"}
APP_OSAL_ALLOWED = ("osal_sleep", "osal_tick_ms", "osal_now_ms", "osal_delay_ms")
BEGINNER_FILES = ("main.c", "main.cpp")

# 入口胶水文件名特征：这些文件允许碰寄存器和 HAL
GLUE_NAME = re.compile(r"(^main\.c$|^startup_.*|^system_.*|.*_it\.c$|.*_irq\.c$|.*_irq_handler\.c$)")

CMSIS_WHITELIST = {"SystemCoreClock", "uwTick", "uwTickPrio", "uwTickFreq"}

ROLE_FILE_NAMES = (".arch-roles", "arch-roles.txt")

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

HAL_CALL = re.compile(r"\b(?:__)?HAL_(?!Delay)[A-Za-z0-9_]+\s*\(")
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

# ISR 内禁止出现的调用
ISR_BLOCKING = re.compile(
    r"\b(HAL_Delay|osDelay|osDelayUntil|vTaskDelay|vTaskDelayUntil"
    r"|xQueueReceive|xSemaphoreTake|osMutexWait"
    r"|delay_ms|delay_us|mdelay|udelay"
    r"|printf|fprintf|sprintf|snprintf|vsnprintf"
    r"|malloc|calloc|realloc|free)\s*\("
)
ISR_NAME = re.compile(r"(IRQHandler$|_isr$|_ISR$|^isr_|_isr_|_IRQHandler)")

SECTION_ATTR = re.compile(r'__attribute__\s*\(\s*\(\s*section\s*\(\s*"([^"]+)"')
LD_SECTION = re.compile(
    r"(?m)^\s*(\.[A-Za-z0-9_.]+)\s+(0x[0-9A-Fa-f]{6,})\s*(\(NOLOAD\))?\s*:"
)
LD_WILDCARD = re.compile(r"\*\(\s*(\.[A-Za-z0-9_.*?\[\]]+)\s*\)")
SECTION_NAME_OK = re.compile(r"^\.(data|bss|rodata|text)\b")

HAL_DELAY = re.compile(r"\bHAL_Delay\s*\(")
SOFT_DELAY = re.compile(r"\b(?:delay_ms|delay_us|mdelay|udelay|osal_sleep_ms)\s*\(")

SKIP_DIRS = {
    ".git", "build", "Build", "Debug", "Release", "Objects", "Listings", ".vscode",
    "node_modules", "Drivers", "Middlewares", "CMSIS", "tests", ".trae-html-share-packages",
}


# ---------------------------------------------------------------- 文本处理


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
    """把注释与字符串内容替换为空格，保留行数。#include 行保留原始引号内容。"""
    out = []
    in_block = False
    for line in text.splitlines():
        if in_block:
            end = line.find("*/")
            if end >= 0:
                in_block = False
                out.append(" " * (end + 2) + _mask_one_line(line[end + 2:])[0])
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


# ---------------------------------------------------------------- 角色解析


def load_roles(root: str, roles_path: str | None):
    """返回 [(pattern, role)]，没有清单则返回 None。"""
    path = roles_path
    if path is None:
        for name in ROLE_FILE_NAMES:
            candidate = os.path.join(root, name)
            if os.path.isfile(candidate):
                path = candidate
                break
    if path is None:
        return None
    if not os.path.isfile(path):
        raise SystemExit(f"角色清单不存在：{path}")
    rules = []
    with open(path, encoding="utf-8", errors="ignore") as fh:
        for no, line in enumerate(fh, 1):
            raw = line.split("#", 1)[0].strip()
            if not raw or "=" not in raw:
                continue
            pattern, role = raw.rsplit("=", 1)
            pattern, role = pattern.strip(), role.strip()
            if role not in ROLES:
                raise SystemExit(f"{path}:{no} 未知角色 {role!r}，可用：{', '.join(ROLES)}")
            rules.append((pattern.replace("\\", "/"), role))
    return rules


def match_role(rel: str, rules) -> str:
    base = rel.rsplit("/", 1)[-1]
    for pattern, role in rules:
        target = base if "/" not in pattern else rel
        if fnmatch.fnmatch(target, pattern):
            return role
    return ""


def dir_role(rel: str) -> str:
    for part in rel.split("/")[:-1]:
        if part in LAYERS:
            return part
    return ""


def is_glue(rel: str) -> bool:
    return bool(GLUE_NAME.match(rel.rsplit("/", 1)[-1]))


def iter_c_files(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if fn.endswith((".c", ".h")):
                yield os.path.join(dirpath, fn)


# ---------------------------------------------------------------- 结构分析


def _skip_ws(code: str, i: int) -> int:
    while i < len(code) and code[i] in " \t\r\n":
        i += 1
    return i


def check_nesting(code: str):
    """只统计"决策型" if（带 else 分支）的嵌套；单分支守卫不计。

    统计全部 if 会把字模渲染、像素扫描这类"双层 for + 单分支 if"判成 4 层，
    在真实工程上制造大量误报。这里改成只认真正的决策金字塔。
    """
    pairs = {}
    stack = []
    for i, ch in enumerate(code):
        if ch == "{":
            stack.append(i)
        elif ch == "}" and stack:
            pairs[stack.pop()] = i

    blocks = []
    for m in re.finditer(r"\bif\b", code):
        j = _skip_ws(code, m.end())
        if j >= len(code) or code[j] != "(":
            continue
        depth = 0
        k = j
        while k < len(code):
            if code[k] == "(":
                depth += 1
            elif code[k] == ")":
                depth -= 1
                if depth == 0:
                    break
            k += 1
        k = _skip_ws(code, k + 1)
        if k >= len(code) or code[k] != "{" or k not in pairs:
            continue
        if code.startswith("else", _skip_ws(code, pairs[k] + 1)):
            blocks.append((k, pairs[k]))

    issues = []
    for open_at, close_at in blocks:
        depth = 1 + sum(1 for o, c in blocks if o != open_at and o < open_at < c)
        if depth > 3:
            issues.append((code.count("\n", 0, open_at) + 1,
                           f"决策型 if 嵌套 {depth} 层（上限 3，单分支守卫不计）"))
    return issues


def find_global_vars(masked: str, raw_lines):
    """文件作用域（花括号深度 0）的可写变量声明。"""
    issues = []
    depth = 0
    for no, line in enumerate(masked.splitlines(), 1):
        code = line.strip()
        opens, closes = line.count("{"), line.count("}")
        raw = raw_lines[no - 1] if no <= len(raw_lines) else ""
        if depth == 0 and code and code.endswith(";") and not code.startswith("#"):
            if (
                "(" in code
                or ")" in code
                or code.startswith("extern")
                or code.startswith("static")
                or code.startswith("const")
                or "__attribute__" in raw
            ):
                depth += opens - closes
                continue
            m = GLOBAL_VAR.match(code)
            if m and m.group(2) not in CMSIS_WHITELIST:
                issues.append((no, f"跨模块全局变量：{m.group(2)}（改走参数中心/只读快照，或改 static）"))
        depth += opens - closes
    return issues


def find_extern_vars(masked: str, raw_lines):
    issues = []
    depth = 0
    for no, line in enumerate(masked.splitlines(), 1):
        code = line.strip()
        raw = raw_lines[no - 1] if no <= len(raw_lines) else ""
        if depth == 0 and code and EXTERN_VAR.match(code) and "__attribute__" not in raw:
            name = re.findall(r"[A-Za-z_]\w*", code)
            if name and name[-1] in CMSIS_WHITELIST:
                depth += line.count("{") - line.count("}")
                continue
            issues.append((no, f"extern 跨模块变量：{code[:40]}"))
        depth += line.count("{") - line.count("}")
    return issues


FUNC_DEF = re.compile(
    r"(?m)^[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{"
)


def iter_functions(masked: str):
    """粗略提取函数定义：(name, 定义行号, 函数体文本, 函数体起始行号)。"""
    for m in FUNC_DEF.finditer(masked):
        name = m.group(1)
        if name in ("if", "for", "while", "switch", "return", "sizeof"):
            continue
        start = m.end() - 1
        depth = 0
        i = start
        while i < len(masked):
            if masked[i] == "{":
                depth += 1
            elif masked[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = masked[start:i + 1]
        yield name, masked.count("\n", 0, m.start()) + 1, body, masked.count("\n", 0, start) + 1


def check_isr(masked: str):
    """ISR 内调用阻塞 / 格式化 / 动态内存。"""
    issues = []
    for name, def_line, body, body_line in iter_functions(masked):
        if not ISR_NAME.search(name):
            continue
        for m in ISR_BLOCKING.finditer(body):
            offset = body_line + body.count("\n", 0, m.start())
            issues.append((offset, f"ISR {name} 内调用 {m.group(1)}()（只允许置标志/发消息/清中断）"))
    return issues


def check_glue_length(raw: str, masked: str, limit: int):
    code_lines = sum(1 for line in masked.splitlines() if line.strip() and not line.strip().startswith("#"))
    if code_lines > limit:
        return [(1, f"入口胶水层 {code_lines} 行代码（上限 {limit}），业务逻辑应下沉到组件层")]
    return []


# ---------------------------------------------------------------- 链接脚本


def collect_linker(root: str):
    sections = []  # (rel, name, addr, addr_int, line, noload)
    wildcards = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if not fn.endswith(".ld"):
                continue
            path = os.path.join(dirpath, fn)
            with open(path, encoding="utf-8", errors="ignore") as fh:
                text = fh.read()
            rel = os.path.relpath(path, root).replace("\\", "/")
            wildcards.extend(LD_WILDCARD.findall(text))
            for m in LD_SECTION.finditer(text):
                line = text.count("\n", 0, m.start()) + 1
                sections.append((rel, m.group(1), m.group(2), int(m.group(2), 16), line,
                                 bool(m.group(3))))
    return sections, wildcards


def check_linker(root: str, c_sections, errors, warns):
    sections, wildcards = collect_linker(root)
    if not sections:
        return

    defined = {}
    by_addr = {}
    for rel, name, addr, value, line, noload in sections:
        defined.setdefault(name, []).append((rel, addr, line, noload))
        by_addr.setdefault(value, []).append((rel, name, addr, line))
        if value % 4 != 0:
            errors.append((rel, line, f"固定地址段 {name} 未 4 字节对齐：{addr}"))

    for value, group in by_addr.items():
        if len(group) > 1:
            detail = "、".join(f"{name}@{rel}" for rel, name, _, _ in group)
            rel, name, addr, line = group[1]
            errors.append((rel, line, f"固定地址段冲突：{detail} 都放在 {addr}"))

    for rel, name, line in c_sections:
        if name in defined:
            if not any(noload for _, _, _, noload in defined[name]):
                warns.append((rel, line, f"固定地址段 {name} 未标 (NOLOAD)，启动时可能被清零"))
            continue
        if SECTION_NAME_OK.match(name):
            continue
        if any(fnmatch.fnmatchcase(name, pat) for pat in wildcards):
            continue
        errors.append((rel, line,
                       f"固定地址段 {name} 未在链接脚本定义（会被通配吞掉或落到默认段）"))


# ---------------------------------------------------------------- 主流程


def main():
    parser = argparse.ArgumentParser(description="四层架构启发式静态检查")
    parser.add_argument("root", help="工程根目录")
    parser.add_argument("--roles", default=None, help="角色清单文件路径")
    parser.add_argument("--glue-max-lines", type=int, default=200, help="入口胶水文件代码行上限")
    parser.add_argument("--max-per-file", type=int, default=5, help="未分层文件每文件最多提示条数")
    parser.add_argument("--no-linker", action="store_true", help="跳过链接脚本检查")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    if not os.path.isdir(root):
        print(f"路径不存在：{root}", file=sys.stderr)
        return 2

    rules = load_roles(root, args.roles)
    header_index = build_header_index(root, rules)

    errors, warns = [], []
    unlayered_hits = 0
    unlayered_files = 0
    c_sections = []

    for path in sorted(iter_c_files(root)):
        rel = os.path.relpath(path, root).replace("\\", "/")
        layer = match_role(rel, rules) if rules else ""
        if not layer:
            layer = dir_role(rel)
        if not layer and is_glue(rel):
            layer = "glue"

        with open(path, encoding="utf-8", errors="ignore") as fh:
            raw = fh.read()
        masked = mask_comments(raw)
        raw_lines = raw.splitlines()

        for m in SECTION_ATTR.finditer(raw):
            c_sections.append((rel, m.group(1), raw.count("\n", 0, m.start()) + 1))

        local_hits = 0
        for no, line in enumerate(masked.splitlines(), 1):
            raw_line = raw_lines[no - 1] if no <= len(raw_lines) else ""
            if "arch-allow" in raw_line:
                continue
            code = line

            if layer in ("", "glue"):
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

            if layer in UPPER_LAYERS:
                if HAL_DELAY.search(code):
                    errors.append((rel, no, "阻塞延时：HAL_Delay（应基于时基非阻塞）"))
                elif SOFT_DELAY.search(code):
                    warns.append((rel, no, "app/组件层出现阻塞延时（确认是否应改为非阻塞时基）"))

            if layer in NON_BSP_LAYERS and path.endswith(".h"):
                m = VENDOR_TYPE.search(code)
                if m:
                    errors.append((rel, no, f"厂商类型泄漏：{m.group(1)}"))

            for m in re.finditer(r'#\s*include\s*[<"]?([^>"]+)[>"]', code):
                target = m.group(1).replace("\\", "/").split("/")[-1]
                tgt_layer = header_index.get(target)
                if tgt_layer and LAYER_DEPTH.get(tgt_layer, -1) > LAYER_DEPTH.get(layer, 99):
                    errors.append((rel, no, f"反向依赖：{layer}/ 包含 {tgt_layer}/ 的 {target}"))
                    break

        if layer == "":
            if local_hits:
                unlayered_files += 1
            continue

        if layer == "glue":
            errors.extend((rel, no, msg) for no, msg in check_glue_length(raw, masked, args.glue_max_lines))
        else:
            if path.endswith(".c") and layer in NON_BSP_LAYERS:
                errors.extend((rel, no, msg) for no, msg in find_global_vars(masked, raw_lines))
                errors.extend((rel, no, msg) for no, msg in find_extern_vars(masked, raw_lines))
            if layer != "bsp":
                errors.extend((rel, no, msg) for no, msg in check_nesting(masked))

        if path.endswith(".c"):
            for no, msg in check_isr(masked):
                raw_line = raw_lines[no - 1] if no <= len(raw_lines) else ""
                if "arch-allow" not in raw_line:
                    errors.append((rel, no, msg))

    if not args.no_linker:
        check_linker(root, c_sections, errors, warns)

    errors.sort(key=lambda x: (x[0], x[1]))
    warns.sort(key=lambda x: (x[0], x[1]))
    for rel, no, msg in errors:
        print(f"[ERROR] {rel}:{no} {msg}")
    for rel, no, msg in warns:
        print(f"[WARN ] {rel}:{no} {msg}")

    if unlayered_files:
        print(
            f"[NOTE ] 另有 {unlayered_files} 个未分层文件、{unlayered_hits} 处平台调用未计入分层检查；"
            "把它们放进 app/ components/ osal/ bsp/，或用 .arch-roles 声明角色"
        )
    if rules:
        print(f"[NOTE ] 已按角色清单 {args.roles or ROLE_FILE_NAMES[0]} 判定层归属")

    print(f"\n合计 {len(errors)} ERROR / {len(warns)} WARN（扫描目录：{root}）")
    return 1 if errors else 0


def build_header_index(root: str, rules):
    """头文件名 → 所在层。"""
    index = {}
    for path in iter_c_files(root):
        if not path.endswith(".h"):
            continue
        rel = os.path.relpath(path, root).replace("\\", "/")
        layer = match_role(rel, rules) if rules else ""
        if not layer:
            layer = dir_role(rel)
        if not layer:
            continue
        index.setdefault(os.path.basename(path), layer)
    return index


if __name__ == "__main__":
    sys.exit(main())
