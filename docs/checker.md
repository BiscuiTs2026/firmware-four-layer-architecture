# 检查器使用说明

`skill/scripts/check_layering.py` 是启发式静态检查，只做文本与结构分析，不编译、不解析 AST。
它抓的是“层归属、依赖方向、可机检的时序/内存布局”这类结构问题；语法、类型与运行时行为不在范围内。

## 用法

```bash
python3 skill/scripts/check_layering.py <工程根目录> [选项]
```

| 选项 | 默认 | 说明 |
|---|---|---|
| `--roles PATH` | 自动找 `.arch-roles` / `arch-roles.txt` | 角色清单路径 |
| `--glue-max-lines N` | 200 | 入口胶水文件代码行上限 |
| `--max-per-file N` | 5 | 未分层文件每文件最多提示条数 |
| `--no-linker` | 关 | 跳过链接脚本固定地址段检查 |
| `--allow-unlayered` | 关 | 允许工程完全没有分层信息（默认判 ERROR） |
| `--version` | 关 | 打印版本与来源仓、来源提交，不需要工程根目录 |
| `--lock PATH` | 关 | 与使用方锁文件比对版本/来源，不一致退出 1；需配合 `--version` |

退出码：`0` 无 ERROR，`1` 存在 ERROR，`2` 用法或路径错误。

`--version` 输出的 `checker-version` 必须与文件里的 `CHECKER_VERSION` 常量一致，
`tests/run_tests.py` 里有一条回归专门检查这一点以及锁文件的一致/不一致两种退出码。

**没有任何分层信息时默认返回 1。** 这是为了避免“全工程未分层 → 0 ERROR → 绿灯空转”；
确实只想统计未分层文件时，显式加 `--allow-unlayered`。

## 两种判层模式

**目录模式**（无需配置）：路径里含 `bsp/`、`osal/`、`components/`、`app/` 即按目录判层。

**角色清单模式**（扁平 `src/`）：

```ini
# .arch-roles
src/main*.c        = glue
src/hw_*.c         = bsp
src/hw_*.h         = bsp
src/logic_*.c      = components
src/logic_*.h      = components
```

规则：

- 一行一条，`模式 = 层`，`#` 起注释；
- 模式不含 `/` 时只匹配文件名，含 `/` 时匹配相对路径（支持 `*`、`?`，由 `fnmatch` 实现）；
- 自上而下**先匹配先生效**，请把更具体的规则放前面；
- 角色只能是 `bsp` / `osal` / `components` / `app` / `glue` / `common`；
- 目录模式与清单模式同时存在时，清单优先。

**优先级顺序**：角色清单 → 目录判定 → 胶水文件名自动识别（`main.c`、`startup_*`、`system_*`、`*_it.c`、`*_irq.c`）。

- 全工程都不命中 → ERROR，提示检查未生效；
- 角色清单存在但一条都没匹配上 → ERROR；
- 部分文件不命中 → 这些文件只报 WARN，不参与分层约束。

## 检查项

| # | 检查 | 级别 | 作用范围 |
|---|---|---|---|
| 1 | 越层调用 HAL / LL / SPL / RTOS，越层访问寄存器（含 `RCC->APBxENR`、`SysTick->VAL`、`SCB->*`、裸地址 MMIO） | ERROR | `app/`、`components/` |
| 2 | 平台头泄漏 | ERROR | 非 bsp 层；`osal/*.c` 作为移植实现豁免 |
| 3 | 厂商类型泄漏（头文件签名与 `.c` 代码） | ERROR | 非 bsp 层 |
| 4 | 反向依赖：include 上层头、本地 `extern` 声明上层函数、直接调用上层函数（函数原型不计为调用）。例外：`bsp` 包含 `components` 的**仅类型/钩子头**（无平台头、无寄存器/厂商类型、无函数定义）→ WARN；`bsp` 调用 `components` 的 `*_hw_*` 钩子 → WARN；`bsp` 包含 `osal` 头放行，符号级限制见第 17 项 | ERROR / WARN | 全部已分层文件 |
| 5 | 跨模块全局变量（文件作用域可写变量） | ERROR | 非 bsp 层 `.c` |
| 6 | `extern` 跨模块变量 | ERROR | 非 bsp 层 `.c` 与 `.h` |
| 7 | 决策型 `if` 嵌套超过 3 层（带 `else` 的决策金字塔；单分支守卫不计） | ERROR | 非 bsp 层 |
| 8 | 未分层文件的平台调用 | WARN | 未分层文件 |
| 9 | APP 直调 BSP、APP 越权调用 OSAL（只允许 `osal_sleep_ms` / `osal_tick_ms` / `osal_now_ms`） | ERROR | `app/` |
| 10 | 入口胶水层超过行数上限 | ERROR | `glue` |
| 11 | ISR 内调用阻塞 / 格式化 / 动态内存函数 | ERROR | 所有 `.c` |
| 12 | app/组件层出现 `HAL_Delay` / 裸 `delay_ms` 等阻塞延时 | ERROR / WARN | `app/`、`components/` |
| 13 | 固定地址段：同址、未对齐、通配段、C 段未显式定义、声明长度后区间重叠 | ERROR / WARN | 工程级 |
| 14 | 宏间接越层：上层使用下层定义的硬件宏，或上层本地宏展开到下层调用/寄存器 | ERROR | `app/`、`components/` |
| 15 | 唯一入口：第二个 `main.c` | ERROR | 工程级 |
| 16 | 全工程没有分层目录或角色清单 | ERROR | 工程级 |
| 17 | BSP 越权调用 OSAL：只允许 `*_critical_enter` / `*_critical_exit` / `*_tick_inc`，其余 `*osal_*` 调用（如 `osal_delay_ms`） | ERROR | `bsp/` |

第 11 项的阻塞函数名单：`HAL_Delay`、`osDelay`、`vTaskDelay`、`xQueueReceive`、`xSemaphoreTake`、`delay_ms`、`delay_us`、`printf`、`fprintf`、`sprintf`、`snprintf`、`malloc`、`calloc`、`realloc`、`free`。
ISR 函数名匹配 `*_IRQHandler`、`*_isr`、`*_ISR`、`isr_*`，以及 CMSIS 内核中断
`SysTick_Handler`、`NMI_Handler`、`HardFault_Handler`、`MemManage_Handler`、`BusFault_Handler`、
`UsageFault_Handler`、`SVC_Handler`、`DebugMon_Handler`、`PendSV_Handler`。

第 13 项解析 `*.ld` 中形如 `.name 0x20003000 (NOLOAD) :` 的显式地址段：

- 同地址两段、地址未 4 字节对齐 → ERROR；
- 固定地址段体里出现 `*(.name*)` 这类通配 → ERROR（必须写成 `*(.name)`）；
- C 里 `section("...")` 的段名必须在 `.ld` 里有**同名显式定义**，通配覆盖不算定义 → 否则 ERROR；
- 段体标注了长度（`/* size: 0x40 */` 或 `. = . + 0x40;`）时，检查区间是否重叠 → ERROR；
- 已定义但未标 `(NOLOAD)` → WARN。

> 区间重叠只有在声明长度时才可判定。没标长度的段只能查出“同址”，不同起始地址的重叠要人工核对。

## 豁免

```c
SystemCoreClock = 8000000U;                      /* CMSIS 必需符号，默认豁免 */

volatile snap_t g_telemetry __attribute__((section(".telem_ram"), used));  /* 带段属性，默认不报全局变量 */

GPIOA->ODR = 1U;            // arch-allow   （本行全部检查跳过）

uint32_t g_dbg_counter;     // arch-allow:snapshot
```

规则：

- `// arch-allow` 跳过该行的**所有**检查；
- `// arch-allow:snapshot` 额外放行该行的全局变量判定；
- 豁免必须写理由，例如 `// arch-allow: 字模查表循环，非业务嵌套`；
- 一个文件里出现三处以上 `arch-allow`，说明规则或代码结构有问题，应改结构而不是继续加豁免。

## 已知误报与漏报

| 场景 | 现象 | 处理 |
|---|---|---|
| `switch` + `if` 状态机 | 少数分支仍可能被判成决策嵌套 | 确认后该行加 `arch-allow`，或改用表驱动 |
| CMSIS 必需符号 | `SystemCoreClock`、`uwTick` | 已默认豁免 |
| 固定地址快照 | 全局变量告警 | 已按段属性豁免 |
| 函数名与变量同名 | 反向依赖可能误判调用目标 | 用 `// arch-allow` 标注，或改名 |
| 不同地址但未标长度的段重叠 | 检查器查不出 | 补 `/* size: 0xNN */`，或人工核对 |

> 字模 / 像素渲染的 `for → for → if (mask)` 曾是最主要的误报来源，
> 已通过“只统计带 `else` 的决策分支”修掉；回归夹具里保留了一条真决策金字塔来覆盖该检查。

## 接进 CI

```yaml
- name: layering
  run: |
    python3 skill/scripts/check_layering.py .
    python3 tests/run_tests.py
```

建议分层：

- `ERROR` 一律阻断合并；
- `WARN` 首次接入时允许存在，但要在 `TECH_DEBT.md` 登记存量；
- 回归夹具（`tests/`）的期望条数变化必须与检查器改动同一个提交；
- 检查器通过只代表“它覆盖的 17 项”没有问题，不代表规范全部满足。
  初始化顺序、ISR 与主循环契约、回调穿层没有静态检查（见 `rules.md` 第 13 节）。
