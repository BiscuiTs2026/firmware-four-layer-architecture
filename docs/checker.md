# 检查器使用说明

`skill/scripts/check_layering.py` 是启发式静态检查，只做文本与结构分析，不编译、不解析 AST。

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

退出码：`0` 无 ERROR，`1` 存在 ERROR，`2` 用法或路径错误。

## 两种判层模式

**目录模式**（无需配置）：路径里含 `bsp/`、`osal/`、`components/`、`app/` 即按目录判层。
适合新工程，或已经拆过层的旧工程。

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
- 角色只能是 `bsp` / `osal` / `components` / `app` / `glue`；
- 目录模式与清单模式同时存在时，清单优先。

**优先级顺序**：角色清单 → 目录判定 → 胶水文件名自动识别（`main.c`、`startup_*`、`system_*`、`*_it.c`、`*_irq.c`）。
三者都不命中 → 记为“未分层”，只报 WARN，不做任何分层约束。

> 这一点必须理解：**未分层 = 不受约束**。全工程都是未分层时，检查器会给出 0 ERROR，那不是合规，那是空转。README 里的实测数据就是这个现象。

## 检查项

| # | 检查 | 级别 | 作用范围 |
|---|---|---|---|
| 1 | 越层调用 HAL / LL / SPL / RTOS，越层访问寄存器 | ERROR | `app/`、`components/` |
| 2 | 平台头泄漏 | ERROR | 非 bsp 层；`osal/*.c` 作为移植实现豁免 |
| 3 | 厂商类型出现在公共头签名 | ERROR | 非 bsp 层的 `.h` |
| 4 | 反向依赖（下层 include 上层头） | ERROR | 全部已分层文件 |
| 5 | 跨模块全局变量（文件作用域可写变量） | ERROR | 非 bsp 层 `.c` |
| 6 | `extern` 跨模块变量 | ERROR | 非 bsp 层 `.c` |
| 7 | `if` 嵌套超过 3 层 | ERROR | 非 bsp 层 |
| 8 | 未分层文件的平台调用 | WARN | 未分层文件 |
| 9 | APP 直调 BSP、APP 越权调用 OSAL | ERROR | `app/` |
| 10 | 入口胶水层超过行数上限 | ERROR | `glue` |
| 11 | ISR 内调用阻塞 / 格式化 / 动态内存函数 | ERROR | 所有 `.c` |
| 12 | 固定地址段同址冲突；C 段属性未在 `.ld` 定义 | ERROR / WARN | 工程级 |

第 11 项的函数名单：`HAL_Delay`、`osDelay`、`vTaskDelay`、`xQueueReceive`、`xSemaphoreTake`、`delay_ms`、`delay_us`、`printf`、`fprintf`、`sprintf`、`snprintf`、`malloc`、`calloc`、`realloc`、`free`。
函数名匹配 `*_IRQHandler`、`*_isr`、`*_ISR`、`isr_*`。

第 12 项解析 `*.ld` 中形如 `.name 0x20003000 (NOLOAD) :` 的显式地址段，同址两段即报冲突；同时把 C 里 `__attribute__((section("...")), used)` 的段名与 `.ld` 对账。

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

## 已知误报

| 场景 | 现象 | 处理 |
|---|---|---|
| 字模 / 查表渲染循环 | `for → for → if → if` 被算作 4 层 | 该行加 `arch-allow`，或在规范中接受 |
| `switch` + `if` 状态机 | 缩进层级被算成嵌套 | 同上；或改用表驱动 |
| CMSIS 必需符号 | `SystemCoreClock`、`uwTick` | 已默认豁免 |
| 固定地址快照 | 全局变量告警 | 已按段属性豁免 |

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
- 回归夹具（`tests/`）的期望条数变化必须与检查器改动同一个提交。
