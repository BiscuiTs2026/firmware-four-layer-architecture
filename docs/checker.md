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
| `--glue-wait LEVEL` | `warn` | 胶水层忙等/阻塞延时：`off` / `warn` / `error` |
| `--glue-pin LEVEL` | `warn` | 胶水层硬编码引脚/端口：`off` / `warn` / `error` |
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

两者的完整写法与判定口径见 [integration.md](integration.md) 第 3 节。

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
| 18 | 胶水层忙等/阻塞延时：空转等待硬件/标志的 `while`/`for`，`HAL_Delay`、`*delay_ms`、`*delay_us` 等调用 | WARN（可调级） | `glue` 的 `.c` |
| 19 | 胶水层硬编码引脚/端口：`GPIO_PIN_x`、作为实参的 `GPIOx`、裸 `0x400xxxxx` 基址 | WARN（可调级） | `glue` 的 `.c` |
| 20 | 胶水层单个函数代码行超过 `--glue-max-func-lines` | WARN | `glue` 的 `.c` |
| 21 | 胶水层单函数内字符串 `else if` 分支达到 `--glue-dispatch-chain`，提示改注册表 | WARN（可调级） | `glue` 的 `.c` |

第 11 项的阻塞函数名单：`HAL_Delay`、`osDelay`、`vTaskDelay`、`xQueueReceive`、`xSemaphoreTake`、`delay_ms`、`delay_us`、`printf`、`fprintf`、`sprintf`、`snprintf`、`malloc`、`calloc`、`realloc`、`free`。
ISR 函数名匹配 `*_IRQHandler`、`*_isr`、`*_ISR`、`isr_*`，以及 CMSIS 内核中断
`SysTick_Handler`、`NMI_Handler`、`HardFault_Handler`、`MemManage_Handler`、`BusFault_Handler`、
`UsageFault_Handler`、`SVC_Handler`、`DebugMon_Handler`、`PendSV_Handler`。

第 18–21 项只作用于 `glue` 层的 `.c` 文件，默认 WARN：

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
- **21 字符串 `else if` 命令链**：同一函数内条件含双引号字面量的 `else if` 分支数达到阈值即报一条，
  位置在函数签名行。`if … return;` 早退守卫链、`switch`、函数指针表不判。

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


## 接进 CI

本仓库自身：

```yaml
- name: layering
  run: |
    python3 skill/scripts/check_layering.py .
    python3 tests/run_tests.py
```

其它工程引用本检查器（单文件拷进 `tools/`）时，CI 跑两条命令：

```yaml
- name: 检查器版本漂移
  run: python3 tools/check_layering.py --version --lock tools/CHECKER.lock
- name: 分层检查
  run: python3 tools/check_layering.py .
```

接入方式、锁文件格式、升级与回灌流程见 [integration.md](integration.md)。

建议分层：

- `ERROR` 一律阻断合并；
- `WARN` 首次接入时允许存在，但要在 `TECH_DEBT.md` 登记存量；
- 回归夹具（`tests/`）的期望条数变化必须与检查器改动同一个提交；
- 检查器通过只代表“它覆盖的 21 项”没有问题，不代表规范全部满足。
  初始化顺序、ISR 与主循环契约、回调穿层没有静态检查（见 `rules.md` 第 13 节）。
