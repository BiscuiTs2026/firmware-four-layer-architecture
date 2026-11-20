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
| `--glue-max-lines N` | 200 | 入口胶水文件代码行上限；**仅作体积兜底，超限报 WARN**，不承担质量判定 |
| `--glue-wait LEVEL` | `warn` | 胶水层忙等/阻塞延时：`off` / `warn` / `error` |
| `--glue-pin LEVEL` | `warn` | 胶水层硬编码引脚/端口（含 `->IDR`、宏别名）：`off` / `warn` / `error` |
| `--glue-duty LEVEL` | `warn` | 胶水层职责规则（驱动/协议/多分支 switch/ISR 体量/长 `if` 链/板级资源表）：`off` / `warn` / `error` |
| `--glue-if-chain N` | 8 | 触发长 `if/else-if` 链提示的最少分支数 |
| `--glue-max-func-lines N` | 80 | 胶水层单个函数代码行上限；`0` 关闭 |
| `--glue-dispatch LEVEL` | `warn` | 胶水层字符串 `else if` 命令链：`off` / `warn` / `error` |
| `--glue-dispatch-chain N` | 4 | 触发命令链提示的最少字符串 `else if` 分支数 |
| `--max-per-file N` | 5 | 未分层文件每文件最多提示条数 |
| `--no-linker` | 关 | 跳过链接脚本固定地址段检查 |
| `--allow-unlayered` | 关 | 允许工程完全没有分层信息（默认判 ERROR） |
| `--version` | 关 | 打印版本与来源仓、来源提交，不需要工程根目录 |
| `--lock PATH` | 关 | 与使用方锁文件比对版本/来源，不一致退出 1；需配合 `--version` |

退出码：`0` 无 ERROR，`1` 存在 ERROR，`2` 用法或路径错误。

`--version` 输出的 `checker-version` 必须与文件里的 `CHECKER_VERSION` 常量一致，
`tests/run_tests.py` 里有一条回归专门检查这一点以及锁文件的一致/不一致两种退出码。
单文件被其它工程引用时的锁文件格式见 [integration.md](integration.md)。

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

**三条指令**（写在清单顶部，用于非四层工程与带厂商包的工程）：

```ini
alias hal = bsp              # 把工程自己的层名映射到内建层；映射后按内建层深度判定
layers = app, hal            # 声明实际存在的层；未声明的层不参与判定
ignore = third_party/, build-*/   # 不参与扫描的路径，可多行，支持目录前缀与通配
```

- `alias` 的目标只能是内建角色，不支持链式别名；`alias` 必须写在用到它的行之前。
- `layers` 省略时按六层全在判定（原有四层工程行为不变）。声明后：
  - 未声明的层不参与判定；**没有 `components` 时 APP 直调 BSP 放行**（两层工程的唯一判据差异）；
  - 文件解析出的层不在声明内 → ERROR，防止清单与代码脱节；
  - `glue` / `common` 始终可用，不必声明。
- `ignore` 相对工程根，`third_party/` 这类目录名可匹配任意深度的同名目录。
- 报错信息里始终使用内建层名（`alias hal = bsp` 之后显示 `bsp/`），便于上游统一收集误报。

### `common` 层怎么判定

- 角色名就是内建角色 **`common`**，不要另造名字。工程已用 `core` 表示“与硬件无关的核心逻辑”时，
  写 `alias core = common`，检查器按 `common` 判定；`core` 若碰寄存器或厂商头，应映射到 `bsp`。
- 路径约定：目录模式认 `common/`；清单模式写 `src/common_*.c = common`（`.h` 也建议显式列出）。
- 可依赖方向：任何层可以 include `common`；`common` 只能 include C 标准库与同层头。
- 判定（第 26 项）：`common` 出现厂商头 include、厂商/内核符号
  （`HAL_` / `LL_` / `__HAL_` / `NVIC_` / `SysTick` / `SCB`）、寄存器访问（含裸地址 MMIO），
  或包含 `bsp` / `components` / `app` / `osal` / `glue` 的头 → **ERROR**；
  `大写标识符->成员` 的解引用 → **WARN**（纯数据聚合也会长这样，按 ERROR 会误伤；
  不按文件白名单整层豁免——白名单是把规则关掉）。
- 共享契约符号（第 27 项，ERROR）：`common` 层 `.h` 里声明、**且 `common` 里确有实现**的函数，
  全仓**带函数体的定义**只允许 1 处。`;` 结尾的声明、函数指针赋值、宏包装不计；
  契约头必须归到 `common`（否则规则看不到）。不做前缀硬编码，符号集来自契约头。
  实现只落在 `bsp` / `osal` / `components` 的公共 API 头不判——工程常把 `include/pm/*.h`
  全映射成 `common`，否则会把正常公共 API 误判成重复实现（板级工程实测 71 条）。
- `common` 的 `.c` 同样受“文件作用域可写变量”限制（第 5 项）：纯逻辑层不持有跨模块状态，
  仲裁所需状态由调用方传入或放到端口层。
- 夹具：`tests/probe-common-bad`（6 ERROR / 1 WARN）、`tests/probe-common-contract`（1 ERROR）、
  `tests/probe-common-clean`（0 发现）。
- **不做**：ISR 与主循环共享的计数字段是否 32 位 `volatile`——字段名猜不出来，
  只能靠 `ISR_SHARED` 之类显式标注，没有标注判不出（见盲区表第 9 条）。

两者的完整写法与判定口径见 [integration.md](integration.md) 第 3 节。

**优先级顺序**：角色清单 → 目录判定 → 胶水文件名自动识别（`main.c`、`startup_*`、`system_*`、`*_it.c`、`*_irq.c`）。

- 全工程都不命中 → ERROR，提示检查未生效；
- 角色清单存在但一条都没匹配上 → ERROR；
- 部分文件不命中 → 这些文件只报 WARN，不参与分层约束。

## 检查项

| # | 规则名（内部名） | 检查 | 级别 | 实现状态 | 作用范围 |
|---|---|---|---|---|---|
| 1 | `platform_call` | 越层调用 HAL / LL / SPL / RTOS，越层访问寄存器（含 `RCC->APBxENR`、`SysTick->VAL`、`SCB->*`、裸地址 MMIO） | ERROR | 已实现 | `app/`、`components/` |
| 2 | `platform_header` | 平台头泄漏 | ERROR | 已实现 | 非 bsp 层；`osal/*.c` 作为移植实现豁免 |
| 3 | `vendor_type` | 厂商类型泄漏（头文件签名与 `.c` 代码） | ERROR | 已实现 | 非 bsp 层 |
| 4 | `reverse_dep` | 反向依赖：include 上层头、本地 `extern` 声明上层函数、直接调用上层函数（函数原型不计为调用）。例外：`bsp` 包含 `components` 的**仅类型/钩子头**（无平台头、无寄存器/厂商类型、无函数定义）→ WARN；`bsp` 调用 `components` 的 `*_hw_*` 钩子 → WARN；`bsp` 包含 `osal` 头放行，符号级限制见第 17 项 | ERROR / WARN | 已实现 | 全部已分层文件 |
| 5 | `global_var` | 跨模块全局变量（文件作用域可写变量） | ERROR | 已实现 | 非 bsp 层 `.c` |
| 6 | `extern_var` | `extern` 跨模块变量 | ERROR | 已实现 | 非 bsp 层 `.c` 与 `.h` |
| 7 | `nesting` | 决策型 `if` 嵌套超过 3 层（带 `else` 的决策金字塔；单分支守卫不计） | ERROR | 已实现（3.3.1 扩展到 glue） | 非 bsp 层 |
| 8 | `unlayered` | 未分层文件的平台调用 | WARN | 已实现 | 未分层文件 |
| 9 | `app_cross_layer` | APP 直调 BSP、APP 越权调用 OSAL（只允许 `osal_sleep_ms` / `osal_tick_ms` / `osal_now_ms`） | ERROR | 已实现（3.3.1 改按定义层） | `app/` |
| 10 | `glue_max_lines` | 入口胶水层超过 `--glue-max-lines` | WARN（体积兜底，不承担质量判定） | 已实现（3.3.1 降为 WARN） | `glue` |
| 11 | `isr_blocking` | ISR 内调用阻塞 / 格式化 / 动态内存函数 | ERROR | 已实现 | 所有 `.c` |
| 12 | `blocking_delay` | app/组件层出现 `HAL_Delay` / 裸 `delay_ms` 等阻塞延时 | ERROR / WARN | 已实现 | `app/`、`components/` |
| 13 | `fixed_section` | 固定地址段：同址、未对齐、通配段、C 段未显式定义、声明长度后区间重叠 | ERROR / WARN | 已实现 | 工程级 |
| 14 | `macro_leak` | 宏间接越层：上层使用下层定义的硬件宏，或上层本地宏展开到下层调用/寄存器 | ERROR | 已实现 | `app/`、`components/` |
| 15 | `unique_main` | 唯一入口：第二个 `main.c` | ERROR | 已实现 | 工程级 |
| 16 | `no_roles` | 全工程没有分层目录或角色清单 | ERROR | 已实现 | 工程级 |
| 17 | `bsp_osal` | BSP 越权调用 OSAL：只允许 `*_critical_enter` / `*_critical_exit` / `*_tick_inc`，其余 `*osal_*` 调用（如 `osal_delay_ms`） | ERROR | 已实现 | `bsp/` |
| 18 | `glue_wait` | 胶水层忙等/阻塞延时：空转等待硬件/标志的 `while`/`for`，`HAL_Delay`、`*delay_ms`、`*delay_us` 等调用 | WARN（可调级） | 已实现 | `glue` 的 `.c` |
| 19 | `glue_pin` | 胶水层硬编码引脚/端口：`GPIO_PIN_x`、作为实参的 `GPIOx`、裸 `0x400xxxxx` 基址、`->IDR`、宏别名引脚；ISR 函数体内读 `->IDR` 不判 | WARN（可调级） | 已实现（3.3.1 补 IDR/别名） | `glue` 的 `.c` |
| 20 | `glue_max_func_lines` | 胶水层单个函数代码行超过 `--glue-max-func-lines` | WARN | 已实现 | `glue` 的 `.c` |
| 21 | `glue_dispatch` | 胶水层单函数内**含字符串字面量的 `else if` 分支数**达到 `--glue-dispatch-chain`，提示改注册表 | WARN（可调级） | 已实现（语义已写死） | `glue` 的 `.c` |
| 22 | `glue_duty` | 胶水层职责：实现外设驱动（G1）、常驻协议分帧（G2）、多分支 `switch`（G3）、ISR 体量超限（G5）、长 `if/else-if` 链（G10，与第 21 项去重）、定义板级资源表（G11） | WARN（`--glue-duty` 可调级） | 已实现（3.3.1） | `glue` 的 `.c` |
| 23 | `glue_isr_business` | ISR 里做业务：ISR 调用 components / app 的非 `*_hw_*` 函数（G4） | ERROR（不可关） | 已实现（3.3.1） | `glue` 的 `.c` |
| 24 | `glue_include` | 其它层 include 入口胶水层的文件/头（G6） | ERROR（不可关） | 已实现（3.3.1） | 全部已分层文件 |
| 25 | `glue_nesting` | 胶水层 `if` 嵌套：与其他层同口径，带 `else` 的决策型 `if` 嵌套 > 3 层（G9） | ERROR | 已实现（3.3.1） | `glue` |
| 26 | `common_purity` | `common` 层：厂商头 / 厂商与内核符号（`HAL_`/`LL_`/`__HAL_`/`NVIC_`/`SysTick`/`SCB`）/ 寄存器 / 包含任何其它层的头 → ERROR；`大写标识符->成员` → WARN | ERROR / WARN | 已实现（3.3.1） | `common` |
| 27 | `common_contract` | 共享契约符号：`common` 头声明且 `common` 确有实现的函数，全仓带函数体的定义只允许 1 处 | ERROR | 已实现（3.3.1） | 全仓 |
| — | `glue_state_use`（G8，未实现） | glue 文件作用域变量的用途 | 盲区 | **不实现** | 人工评审 |
| — | `glue_pin_alias_other`（G12 残余） | 带括号/拼接/`enum`/`const` 形式的引脚别名 | 盲区 | **不实现** | 人工评审 |

第 11 项的阻塞函数名单：`HAL_Delay`、`osDelay`、`vTaskDelay`、`xQueueReceive`、`xSemaphoreTake`、`delay_ms`、`delay_us`、`printf`、`fprintf`、`sprintf`、`snprintf`、`malloc`、`calloc`、`realloc`、`free`。
ISR 函数名匹配 `*_IRQHandler`、`*_isr`、`*_ISR`、`isr_*`，以及 CMSIS 内核中断
`SysTick_Handler`、`NMI_Handler`、`HardFault_Handler`、`MemManage_Handler`、`BusFault_Handler`、
`UsageFault_Handler`、`SVC_Handler`、`DebugMon_Handler`、`PendSV_Handler`。

第 18–26 项只作用于 `glue` 层（第 24 项作用于 include 方，第 26 项作用于 `common`），除明确标 ERROR 的项外默认 WARN：

- **18 忙等/阻塞延时**：阻塞延时按调用名识别，允许工程前缀（`pm_osal_delay_ms`、`bsp_delay_us` 都算），
  名单为 `HAL_Delay`、`osDelay`、`osDelayUntil`、`vTaskDelay`、`vTaskDelayUntil`、`delay_ms`、`delay_us`、`mdelay`、`udelay`；
  本地同名函数的定义行与 `;` 结尾的原型声明不算调用。忙等只认循环体为空（或只有 `;`）且条件里出现
  `->` 或 `ready`/`busy`/`flag`/`done`/`status`/`state` 语义的 `while`/`for`。
  故障出口函数（名字匹配 `error_handler` / `error_loop` / `fault_*` / `panic*` / `hardfault*` / `default_handler`，不区分大小写）里的第 18 项豁免。
  预处理行（`#` 开头）不参与 18/19 项。
- **19 硬编码引脚/端口**：`GPIO_PIN_<数字>`、不紧跟 `->` 的 `GPIOA`–`GPIOK`（作为实参传入）、裸 `0x400xxxxx`。
  `GPIOx->REG` 的寄存器读写不在此列。
- **20 单函数过长**：函数体内非空、非纯 `{` / `}`、非预处理行的行数，超过阈值即报。判的是“代码行”，
  空行与整行注释不计。
- **21 字符串 `else if` 命令链**：**计数单位是含字符串字面量的 `else if` 分支个数**（不是字符串比较函数的调用次数，也不含首个 `if`）。
  同一函数内达到阈值即报一条，位置在函数签名行。`if … return;` 早退守卫链、`switch`、函数指针表不判。
- **22 胶水层职责（G1–G3/G5/G10）**：
  - G1 驱动实现：**命中单位是函数个数**。非 init / 非 ISR / 非故障出口函数体内外设寄存器访问
    （`->REG` 或裸地址 MMIO 强转）≥ 3 次，该函数计 1 条；消息里给出访问次数。
  - G2 协议分帧：单函数 `strcmp` / `strncmp` / `ci_equal` ≥ 3 次。
  - G3 多分支 `switch`：单函数 `switch` 的 `case` ≥ 4 个（`default` 不计）。命中只说明判定留在入口，
    消息按“switch 分支”表述，不推断是不是状态机。
  - G5 ISR 体量：ISR 函数体代码行 > 40。
  - G10 长 `if` 链：同一 `if/else-if` 链分支数 ≥ `--glue-if-chain`（默认 8），任意条件，`switch` 不计。
    同一函数已由第 21 项报过的，G10 跳过，不重复报。
- **23 ISR 做业务**：`glue` 的 ISR 调用定义在 `components` / `app` 的函数即 ERROR；
  `*_hw_*` 钩子、`*FromISR`、`bsp` 函数、`osal` 临界区放行。经宏或函数指针的间接调用判不出（见盲区）。
- **24 胶水层不得被 include**：任何非 glue 层 include 了 glue 的文件/头 → ERROR。
- **25 胶水层 if 嵌套**：与第 7 项同口径，带 `else` 的决策型 `if` 嵌套 > 3 层 → ERROR。
- **26 common 层**：见上文“`common` 层怎么判定”。
- **27 共享契约符号**：`common` 头声明、且 `common` 确有实现 → 全仓只允许一处带体定义；
  声明（`;`）、函数指针赋值、宏包装不计。声明按文件作用域的 `;` 拼段后解析，跨行原型
  （返回类型独占一行、参数表换行、`static` / `const` 前导）同样识别。数量不符报在契约头声明行。
  实现只落在 bsp/osal/components 的公共 API 头不判（避免把所有公共契约头映射成 common 的工程满屏误报）。

G8（胶水层文件作用域变量的用途）**不实现**，理由见 `rules.md` §1.4 与本文末尾盲区表。

`arch-allow` 对逐行规则（18/19）看该行行尾；对函数级规则（20/21）看函数签名行行尾。

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
| 故障出口函数（`Error_Handler`、`error_loop`、`fault_*`、`panic*`）里的阻塞延时/忙等 | 第 18 项不判 | 这些函数不再返回调度，阻塞属于设计；第 19 项引脚规则仍生效 |
| 不同地址但未标长度的段重叠 | 检查器查不出 | 补 `/* size: 0xNN */`，或人工核对 |
| 空主循环 `while (1) {}`、`for (;;) {}`、`for (volatile i…) {}` 软件延时 | 不判忙等 | 有意保守：只判“循环体为空且条件含寄存器或 ready/busy/flag 语义”的循环；主循环与故障出口不误伤 |
| 胶水层 `GPIOA->ODR` 之类寄存器操作 | 不判硬编码引脚 | 胶水层本来就允许寄存器操作；第 19 项只抓引脚**映射**（`GPIO_PIN_x`、作为实参的 `GPIOx`、裸基址） |
| CubeMX/HAL 生成工程里的 `GPIO_PIN_x`、`HAL_GPIO_Init(GPIOA,…)` | 第 19 项会命中生成代码 | 检查器不区分生成行与手写行；接入时把生成文件 `ignore` 掉，只扫自有胶水 |
| 自定义命名的阻塞等待（如 `wait_ready()`、`poll_status()`） | 第 18 项不判 | 只按名称后缀识别已知延时函数；发现漏报请提夹具回灌 |
| `if (eq(cmd,"A")) { return; }` 早退守卫链、`switch`、函数指针注册表 | 第 21 项不判 | 只抓长 `else if` 链，避免把合法早退写法判成命令分发 |

> 字模 / 像素渲染的 `for → for → if (mask)` 曾是最主要的误报来源，
> 已通过“只统计带 `else` 的决策分支”修掉；回归夹具里保留了一条真决策金字塔来覆盖该检查。

### 已知漏报（明确不做，不是没发现）

下面六条是检查器**有意不判**的，写在这里而不是留在讨论记录里。判不出来的必须写在纸面上，
这是这套规则自己的信用边界；不要用“看起来没问题”代替“没有检查”。

| # | 不判的形态 | 原因 | 替代手段 |
|---|---|---|---|
| 1 | 第 18 项不做成 ERROR（默认 WARN） | 纯文本分不清“上电初始化等待”与“运行期阻塞”；同样一句 `HAL_Delay(10)` 在初始化里可能合理，在调度里就是设计错误 | 第 18 项只提示，是否进 gate 由使用方按 [integration.md](integration.md) 4.2 的门槛决定 |
| 2 | 不抓所有 `while` / `for` | 排空 FIFO、软件计数延时、主循环 `while (1)` 都会误伤；全抓等于让所有人第一天就加 `arch-allow` | 只抓“循环体为空且条件含寄存器或 `ready`/`busy`/`flag` 语义”的循环 |
| 3 | 不抓 `GPIOx->REG` 寄存器读写 | 胶水层本来就允许寄存器操作，判它等于把第 1 项的范围搬到胶水层 | 第 19 项只抓引脚**映射**：`GPIO_PIN_x`、作为实参的 `GPIOx`、裸 `0x400xxxxx` |
| 4 | 不抓 `if (…) return;` 早退守卫链、`switch`、函数指针表 | 这三种是合法的分发写法，按 `else if` 计数会把好代码判成坏代码 | 第 21 项只抓同函数内比较字符串字面量的长 `else if` 链 |
| 5 | 不自动跳过含 `USER CODE` 的生成 `main.c` | 自动跳过会连同写在 `USER CODE` 区里的业务逻辑一起放过，而这正是胶水层写坏的高发区 | 要求使用方用 `ignore` 显式挡掉整个生成文件，手写胶水单独成文件后接受全部检查 |
| 6 | 不识别自定义命名的阻塞函数 | `wait_ready()`、`poll_status()`、`spin_until()` 没有任何命名约定可依，按名字判定必然误报 | 第 18 项只认已知延时名单（允许工程前缀）与空体忙等特征；发现漏报按 [integration.md](integration.md) 第 4 节回灌夹具 |

第 1、2、6 条合起来就是一句话：**“忙等”在静态文本里没有可靠签名**。检查器能稳定抓到的是
“已知延时函数的调用”和“空转等寄存器/标志”两类，其余靠人工评审；这条限制不会因为版本升级消失。

### 盲区清单（无法静态判定的形态）

使用方只需读这一节。下列形态检查器**不判**，不代表代码没问题：

| # | 盲区 | 为什么不判 | 替代手段 |
|---|---|---|---|
| 1 | **G8：glue 文件作用域变量的用途** | 同一形态既可能是合法句柄/初始化标志/ISR 交棒标志，也可能是跨命令业务缓存；纯文本判不出用途。按形态报 WARN 在板级工程一次命中 43 条，规则不可用 | 人工评审；G8 不实现、不设豁免语法 |
| 2 | **函数定义提取的残余边界** | 3.3.1 已按顶层花括号配对提取，支持前导/后置 `__attribute__`、跨行签名、`static inline`；但 K&R 旧式参数、宏生成的函数、`#if` 分支里的定义仍可能漏检，漏检函数会绕过第 20/21 项与 G1/G2/G3/G5/G10 | 用夹具回灌；命中异常稀疏时先怀疑提取，再怀疑规则 |
| 3 | **宏别名的其它形态** | G12 只认同文件单行 `#define NAME GPIOx` / `GPIO_PIN_n`；`#define LED_PORT (GPIOA)`（带括号）、多层宏拼接、`enum` / `const` 别名判不出 | 人工评审；统一改用端口层句柄 |
| 4 | **G4 的间接调用** | ISR 经宏、函数指针或注册表调用 components/app 函数时，文本里没有“ISR → 业务函数”的直接调用边 | 人工评审；注册表声明处加注释说明调用上下文 |
| 5 | **`main.cpp` 不在胶水判定范围** | 胶水文件名只认 `main.c`、`startup_*`、`system_*`、`*_it.c`、`*_irq.c`；`main.cpp` 不会被判成 glue，也就不会触发第 18–25 项 | C++ 工程在 `.arch-roles` 显式写 `src/main.cpp = glue` |
| 6 | **初始化名豁免漏运行期函数** | G1 按名字排除 `*_init*` / `*_config*` / `*_setup*` / `*_clock*`；运行期函数若叫 `relay_setup_cycle` 会被当作初始化放过 | 人工评审；避免用初始化词根命名运行期函数 |
| 7 | **ISR 里写文件作用域状态、宏内联业务** | 与 G8 同源：变量用途与宏展开后的调用链都判不出 | 人工评审 |
| 8 | **资源仲裁函数全仓唯一（语义层）** | 第 27 项只能抓“契约头声明 + 同名带体定义”这一种；换名字的语义重复、跨模块的功能复制判不出 | 人工评审；契约统一收敛到 `common` 头 |
| 9 | **ISR 共享计数字段的位宽与 `volatile`** | 应由 ISR 写、主循环读的字段必须 32 位且 `volatile`，但字段名猜不出来，只能靠显式标注（如 `ISR_SHARED`）；没有标注判不出 | 人工评审；可选规则包（需工程统一标注） |
| 10 | **ISR 内读 `->IDR` 不判** | 判上下沿是标准中断分发（A1 允许），与 G1 的 ISR 豁免同口径；G7 只报 ISR 之外的业务性读 | 人工评审：确认该 ISR 只做分发，不夹带业务 |
| 11 | **停用功能仍在初始化 / 使能中断 / 被 ISR 使用** | “功能是否停用”是组件 `.c` 里的本地宏；“初始化调用”“`NVIC_EnableIRQ`”“ISR 体”分散在 glue 与 bsp 的不同文件。纯文本拿不到跨文件宏值，也不建模 `#if` 区域，按 `*_init` / `EXTI*_IRQHandler` 之类前缀推断必然误伤 | 人工评审；将来以显式锚落地（见下节“待立项”），**不按名字猜** |

### 待立项：停用功能一致性（拟第 28 项，暂不实现）

**立项前提**：板端先把“停用功能的初始化与中断”修干净——`pm_sonar_init()`、EXTI 使能、`EXTI15_10_IRQHandler` 一并放进 `#if DIST_TIME_BASE_READY`，重新启用时补 `NVIC_SetPriority`——再立项。否则规则一上线就会把板端 CI 打红。

**最小切片（显式锚、零猜测）**：

- `.arch-roles` 增三条可选声明，未声明就不判：
  ```
  feature DIST = off
  feature-symbol DIST = pm_sonar_init
  feature-irq DIST = EXTI15_10_IRQn
  ```
- 第 28 项 `feature_gate`：仅对 `off` 特性，检查 `feature-symbol` 是否仍有调用点（复用 `func_index` + `CALL_NAME`，排除定义行与原型），以及 `feature-irq` 是否仍有 `NVIC_EnableIRQ` / `HAL_NVIC_EnableIRQ`；命中为 ERROR。
- **前置依赖**：检查器需先具备 `#if` 区域跳过语义。否则合法修复（`#if` 包住调用）仍会被文本扫到，规则会把修好的代码继续报错。
- 夹具三个：`bad`（off + 调用 + EnableIRQ，期望 2 ERROR）、`clean`（on，0 发现）、`guarded`（调用在 `#if` 里，0 发现）。第三个过不了，就不能算“能自证”。
- 级别与开关（默认 WARN/ERROR、是否并入 `--glue-duty`）立项时再定；本节只留档，不改现有判据。

> 另见 `rules.md` 第 13 节：初始化顺序、ISR 与主循环契约、回调穿层、函数定义提取残余边界等。


## 接进 CI

本仓库自身：

```yaml
- name: layering
  run: python3 tests/run_tests.py
```

**只跑 `tests/run_tests.py`，不要对仓库根跑 `check_layering.py .`。**
本仓库的 `examples/` 下每个示例各自带一个 `main.c`，全仓扫描会命中第 15 项（唯一入口）
6 条 ERROR——那是示例隔离的预期结果，不是缺陷；`run_tests.py` 会分别扫描每个夹具并核对条数。

其它工程引用本检查器（单文件拷进 `tools/`）时，CI 跑两条命令：

```yaml
- name: 检查器版本漂移
  run: python3 tools/check_layering.py --version --lock tools/CHECKER.lock
- name: 分层检查
  run: python3 tools/check_layering.py .
```

接入方式、锁文件格式、升级与回灌流程见 [integration.md](integration.md)。

**推送前自查（本仓约定）**：改完检查器或夹具，先本地照抄 CI 的两条命令各跑一遍，再推送：

```bash
python tests/run_tests.py
cmake -S . -B build-host -DFOUR_LAYER_HOST_CHECK=ON && cmake --build build-host --parallel
```

只跑 `tests/run_tests.py` 的默认路径不够：CI 是 Linux + LF 换行，本地是 Windows + CRLF，
目录遍历顺序、换行符、路径大小写都可能只在 CI 上暴露。跨平台输出必须排序后再拼接。

建议分层：

- `ERROR` 一律阻断合并；
- `WARN` 首次接入时允许存在，但要在 `TECH_DEBT.md` 登记存量；
- 回归夹具（`tests/`）的期望条数变化必须与检查器改动同一个提交；
  版本号与该版本的 `CHANGELOG.md` 记录也必须在同一个提交里，不允许先改版本后补记录；
- 检查器通过只代表“它覆盖的 27 项”没有问题，不代表规范全部满足。
  初始化顺序、ISR 与主循环契约、回调穿层没有静态检查（见 `rules.md` 第 13 节）。
