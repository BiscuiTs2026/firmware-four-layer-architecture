# firmware-four-layer-architecture

给 AI 写单片机固件的工程约束：**四层架构 + HAL/裸机双形态 + 可执行检查器**。

多数 AI 生成的固件烂在同一个地方：`main.c` 越写越厚、HAL 和寄存器调用渗进业务代码、换一颗芯片要重写全工程。
这个仓库把这些问题写成规范，并把其中**能静态识别的一部分**做成脚本；不能机检的部分明确标为人工评审或未定义。

> 先说清楚能力边界：检查器是启发式文本分析，不是编译器，也不是 MISRA。
> 它覆盖 17 项结构检查；初始化顺序、ISR 与主循环契约、回调穿层**没有**静态检查（见 [rules.md](rules.md) 第 13 节）。
> 检查器绿灯只代表它覆盖的那些项没问题。

## 它解决什么

| 症状 | 本仓库的约束 | 能否机检 |
|---|---|---|
| 业务逻辑全堆在 `main.c` | 入口胶水层 ≤ 200 行，只做初始化与中断分发 | 能 |
| 组件层直接写 `GPIOA->ODR` | 寄存器 / HAL / LL / SPL 只允许在 BSP 与胶水层（含 `SysTick->VAL`、`SCB->*`、`RCC->APBxENR`、裸地址 MMIO） | 能 |
| 公共头里出现 `UART_HandleTypeDef` | 厂商类型不得出 BSP（头文件与 `.c` 都查） | 能 |
| 模块之间靠 `extern` 变量通信 | 禁 extern 传数据，只读快照除外（`.c` 与 `.h` 都查） | 能 |
| 用宏把下层调用藏起来 | 上层使用下层硬件宏、或上层宏展开到下层调用 | 能 |
| BSP 在本地 `extern` 反调组件 | 反向依赖同时查 include、extern 声明与直接调用 | 能 |
| ISR 里 `printf`、`delay_ms` | ISR 只允许置标志/发消息/清中断；含 `SysTick_Handler` 等内核中断 | 能 |
| 整屏刷新 OLED 把 10 ms 节拍拉长 | 慢外设必须分块非阻塞，主循环有耗时上限 | 不能（需实测） |
| 固定地址回读块互相覆盖 | 段名显式定义、禁止通配段、声明长度后查区间重叠 | 能 |
| 换芯片重写上层 | 目标是只改 BSP 与 OSAL 移植实现 | 部分（类型/头文件泄漏可查，时序假设查不了） |

## 四层

```
APP → components → OSAL → BSP → 硬件
```

| 层 | 目录 | 拥有 | 绝对不能 |
|---|---|---|---|
| APP | `app/` | 流程编排、调组件 API | 碰寄存器、HAL、RTOS API、BSP |
| 组件 | `components/` | 业务逻辑、算法、事件发布 | 碰硬件、跨组件包含内部头 |
| OSAL | `osal/` | 时基、延时、互斥、队列 | 公共头暴露芯片头或 RTOS 头 |
| BSP | `bsp/` | 寄存器、HAL、引脚映射、位时序原语 | 携带业务语义、反调组件 API、调用 OSAL 临界区以外的接口 |
| 胶水 | `main.c` 等 | 初始化、中断分发、调度 | 承载业务逻辑 |

`common/` 放纯类型（`err_t`、枚举），任何层可 include，它不 include 任何层。

位时序（软件 I2C、单总线）由 BSP 自带的 `bsp_delay_us` 完成，不依赖 OSAL；
OSAL 可以基于 BSP 的时基实现毫秒接口。这是规范对“BSP 不能用 OSAL”的正式取舍，见 `rules.md` 2.2 第 5 条。

BSP 另有两个窄例外：为保护任务/ISR 共享状态，可 include OSAL 头并使用
`*_critical_enter` / `*_critical_exit` / `*_tick_inc`（其余 OSAL 调用仍判 ERROR）；
端口层为拿到句柄类型与 ISR 钩子，可包含组件层的仅类型/钩子头（检查器降级为 WARN），见 `rules.md` 2.2 第 6 条与第 3 节。

## 两种工程形态

- **HAL 形态**：STM32CubeMX2 / CubeIDE / SysConfig 生成。规则只允许在 `USER CODE` 区写，不改 `.ioc`、不碰生成文件。
- **裸机形态**：自写启动文件与链接脚本，直接操作寄存器。启动文件与链接脚本只允许人工确认后修改。

两种形态共用同一套分层判据，检查器自动跳过 `Drivers/`、`CMSIS/`、`build/` 等生成目录。

## 快速开始

```bash
# 1. 分层工程：直接扫
python3 skill/scripts/check_layering.py path/to/project

# 2. 扁平 src/ 工程：先写角色清单
cat > .arch-roles <<'EOF'
src/main*.c      = glue
src/i2c_soft.*   = bsp
src/telemetry.*  = components
EOF
python3 skill/scripts/check_layering.py path/to/project

# 3. 回归自检（改检查器后必须跑）
python3 tests/run_tests.py
```

退出码：`0` 无 ERROR，`1` 有 ERROR，`2` 用法错误。

**没有分层目录、也没有角色清单时默认返回 1。** 早期版本会返回 0，导致“什么都没检查却绿灯”；
现在必须显式加 `--allow-unlayered` 才会放行。退出码可以直接用作 CI 门槛。

## 接进别的工程

既有的真实工程往往不按四层建目录，也不该为了装检查器重构一遍。检查器支持**单文件拷出使用**：

```
<你的工程>/
├── .arch-roles              # 层名映射 + alias / layers / ignore 指令
├── tools/
│   ├── check_layering.py    # 从本仓库某个 commit 原样拷入；来源提交要改两处，见 docs/integration.md
│   └── CHECKER.lock         # 同步点：版本、来源仓、来源提交
```

只依赖 Python 3.8+ 标准库，不需要子模块、不需要安装、不需要检出本仓库。CI 两条命令：

```bash
python3 tools/check_layering.py --version --lock tools/CHECKER.lock   # 版本漂移
python3 tools/check_layering.py .                                     # 分层门槛
```

只有两层（`app` + `hal`）的工程用 `.arch-roles` 声明即可，见 [examples/two-layer/](examples/two-layer/)：

```ini
alias hal = bsp
layers = app, hal
```

两层的判定口径（`app` 直调 `hal` 放行、`hal` 反调 `app` 仍是 ERROR）、接入理由、
升级步骤与**规则回灌流程**见 [docs/integration.md](docs/integration.md)。

## 真实效果（作者自己的工程，非构造样例）

在一套 3569 行的裸机工程（STM32F103C8T6，6.6 KB Flash）上实测：

| 配置 | 当时的结果 | 现在的行为 |
|---|---|---|
| 不写角色清单，直接扫 | 0 ERROR，31 条“未分层”WARN | 1 ERROR：检查未生效（除非加 `--allow-unlayered`） |
| 写角色清单后扫 | 35 ERROR，按类别可定位 | 同样会扫出，另有新增的宏/头文件/反向调用检查 |
| 327 行的 `main.c` 工程 | 1 ERROR：胶水层 268 行超限 | 相同 |

35 条里包含 25 条越层寄存器、3 条 `extern` 时基、2 条平台头泄漏、4 条跨模块全局变量、1 条链接脚本段未定义。
它们都是可定位的结构问题；**是否每一条都值得改、改动成本多大，仍需人工判断**。
详细数据与缺陷分类见 [docs/measurements.md](docs/measurements.md)。

## 示例可构建到什么程度

| 示例 | 形态 | 已验证 | 未验证 |
|---|---|---|---|
| `examples/good-hal/` | HAL | 主机侧语法编译（CMake + host stub） | 目标板构建与上板行为 |
| `examples/good-baremetal/` | 寄存器 + 固定地址快照 | 主机侧语法编译 | 目标板构建与上板行为 |
| `examples/flat/` | 扁平 `src/` + `.arch-roles` | 主机侧语法编译 | 目标板构建与上板行为 |
| `examples/two-layer/` | 两层 `app` + `hal` | 检查器零发现 + 主机侧语法编译（arm-none-eabi-gcc 14.3.1，零警告） | 目标板构建与上板行为 |

主机侧编译使用 `tests/host-stubs/` 里的最小 CMSIS/HAL 头，只验证 C 语法、头文件路径与层间接口一致性：

```bash
cmake -S . -B build-host -DFOUR_LAYER_HOST_CHECK=ON
cmake --build build-host
```

没有宿主 gcc、只有 `arm-none-eabi-gcc` 的机器（例如 Windows 上的 Cube 工具链）可以显式指定编译器：

```powershell
cmake -S . -B build-host -G Ninja `
  -DCMAKE_C_COMPILER="$env:LOCALAPPDATA\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin\arm-none-eabi-gcc.exe" `
  -DCMAKE_MAKE_PROGRAM="$env:LOCALAPPDATA\stm32cube\bundles\ninja\1.13.2+st.1\bin\ninja.exe"
cmake --build build-host
```

顶层 `CMakeLists.txt` 已设 `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`，否则交叉编译器在 CMake
的“链接可执行文件”探测阶段会因缺主机 CRT 失败。产物是静态库，不链接、不运行。

**这不是完整固件工程。** 目标板构建需要 STM32CubeF1（CMSIS + HAL）与 `arm-none-eabi-gcc`，
仓库不附带厂商包；把示例接进真实工程时，需要补启动文件、链接脚本、HAL 源文件与工具链文件。
`tests/host-stubs/` 只用于主机侧检查，不能烧录。

## 仓库结构

| 路径 | 作用 |
|---|---|
| `rules.md` | 规范本体，可直接粘进 AI 或存为项目规则文件 |
| `skill/` | 可安装的 Agent Skill（SKILL.md + 细则索引 + 检查器） |
| `examples/good-hal/` | 合规示例：HAL 形态全链条 |
| `examples/good-baremetal/` | 合规示例：寄存器级全链条 + 固定地址快照 |
| `examples/flat/` | 合规示例：扁平 `src/` + `.arch-roles`，不拆目录也能过检查 |
| `examples/two-layer/` | 合规示例：两层 `app` + `hal`，演示 alias / layers 指令 |
| `examples/bad/` | 违规样例：每条规则至少踩一次，附期望输出 |
| `tests/probe/` | 回归夹具：目录分层模式，逐条植入违规 |
| `tests/probe-gaps/` | 对抗夹具：专门覆盖历史漏报（ENR/SysTick/SCB/裸地址/宏/头文件 extern/反向调用） |
| `tests/probe-flat/` | 回归夹具：角色清单模式 |
| `tests/probe-two-layer/` | 回归夹具：两层工程 + alias / layers / ignore |
| `tests/probe-layers-drift/` | 回归夹具：声明层集合与代码不一致 |
| `tests/host-stubs/` | 主机侧语法检查用的最小 CMSIS/HAL 头 |
| `tests/host-build/` | 主机侧 CMake 目标（只编译，不链接不运行） |
| `docs/checker.md` | 检查项、豁免语法、误报与漏报 |
| `docs/integration.md` | 外部工程接入、非四层写法、回灌流程与 gate 门槛 |
| `docs/measurements.md` | 实测数据与缺陷分类 |
| `docs/guide-zh.md` | 设计取舍说明 |

## 检查器覆盖的 17 项

越层调用平台 API（含寄存器与裸地址 MMIO）、平台头泄漏、厂商类型泄漏（`.h` 与 `.c`）、
反向依赖（include / 本地 extern / 直接调用；`bsp` 包含仅类型/钩子头、调用 `*_hw_*` 钩子降为 WARN）、跨模块全局变量、
`extern` 跨模块变量（`.c` 与 `.h`）、决策型 `if` 嵌套超 3 层、未分层文件的平台调用、
APP 直调 BSP / 越权调 OSAL、胶水层超长、ISR 内阻塞调用（含 `SysTick_Handler`）、
app/组件层阻塞延时、固定地址段（同址/对齐/通配/未定义/区间重叠）、宏间接越层、唯一 `main.c`、
全工程无分层信息、BSP 越权调用 OSAL（仅允许临界区与 tick 三个原语）。

豁免语法：行尾 `// arch-allow`（跳过该行），`// arch-allow:snapshot`（额外放行快照声明）。
`SystemCoreClock` 等 CMSIS 必需符号默认豁免。

## 未定义的部分

以下三件事规范没有给出唯一答案，检查器也不检查，遇到要先约定：

1. **初始化顺序**：BSP / OSAL / 组件 / APP 的 `_init` 次序，以及失败后降级还是停机；
2. **ISR 与主循环契约**：用什么同步（`volatile` 标志、原子操作、无锁队列）、临界区归谁、主循环耗时如何验收；
3. **回调穿层**：回调类型、持有者、执行上下文（ISR 还是主循环）、能否在回调里再调下层。

示例采用“OSAL → BSP → 组件 → APP，失败进故障出口；组件只暴露 `_task`/轮询接口”，
但这是示例约定，不是规范。选定方案后请写进项目自己的架构说明。

## 边界

- 这是**启发式**检查，不是编译器，也不是 MISRA 替代品。它抓结构性问题，不抓语法与类型错误。
- 规则里的“唯一例外”都是为真实上板调试留的口子（固定地址快照、入口胶水层）。口子有明确尺寸，不是想放就放。
- 固定地址段的区间重叠只有在段体声明长度（`/* size: 0xNN */`）时才可判定。
- 规范要求按层拆分，但**不要求一次拆完**：新功能进对层，旧代码在下次改动该文件时顺手下沉。

## 许可

MIT，见 [LICENSE](LICENSE)。
