# 把检查器接进别的工程

本文定的是“检查器被别人用”的可执行机制，不是口头约定。适用于已经存在、不想按四层重排目录的真实工程。

三句话概括：

1. 使用方把 `check_layering.py` **单文件**拷进自己的 `tools/`，配一份 `.arch-roles` 与一份 `tools/CHECKER.lock`；
2. 检查器只依赖 Python 标准库，不读本仓库任何其它文件，拷出去就能跑；
3. 规则演进由上游改，使用方按锁文件升版本；误报与漏报走第 4 节的回灌流程。

---

## 1. 为什么是单文件拷贝

| 方案 | 问题 |
|---|---|
| git submodule / subtree | 使用方仓库多为私有工程，不想让完整上游仓库出现在自己的历史里；CI 还要处理 submodule 凭据 |
| pip 包 / 私有索引 | 检查器是单文件、零依赖，为它维护打包与发版流水线不划算；反而多一层安装失败面 |
| 让使用方检出上游整仓 | 使用方 CI 需要额外 checkout 与路径约定，且上游用例目录会被误扫 |
| **单文件拷贝 + 锁文件** | 升级是显式动作，CI 在单个 commit 内可复现，检查器只依赖标准库 |

代价也要写清楚：拷贝出去的文件不会自动跟随上游更新，**必须靠 `CHECKER.lock` 与 `--version --lock` 发现漂移**。
所以锁文件不是可选项，是这套机制的组成部分。

---

## 2. 使用方仓库要放什么

```
<工程根>/
├── .arch-roles               文件 → 层映射，以及 alias / layers / ignore 指令
├── tools/
│   ├── check_layering.py     从上游某个 commit 原样拷入，只改文件头 source-commit 一行
│   └── CHECKER.lock          同步点：版本、来源仓、来源提交、本地补丁说明
└── .github/workflows/arch.yml   CI 门槛（也可用 GitLab CI / 本地 pre-push）
```

### 2.1 文件头与锁文件

拷入的文件头固定三行，其中 `source-commit` 由使用方填写：

```python
# source-repo: https://github.com/BiscuiTs2026/firmware-four-layer-architecture
# checker-version: 3.2.0
# source-commit: self    ← 上游仓库内为 self；拷到使用方仓库后改成上游提交哈希，并在 tools/CHECKER.lock 里登记
```

`tools/CHECKER.lock`（唯一同步点）：

```
# 上游检查器锁文件：改这里等于声明"我升到了某个版本"
checker-version = 3.2.0
source-repo     = https://github.com/BiscuiTs2026/firmware-four-layer-architecture
source-commit   = 3f1c9a2b7d8e4c5f0a1b2c3d4e5f60718293a4b5
synced-at       = 2026-11-18
local-patches   = none
```

`local-patches` 默认写 `none`。真要本地改检查器，必须在这里写清改了什么、为什么、以及是否已向上游提夹具；
否则升级时会被新文件覆盖。

### 2.2 CI 门槛

```yaml
name: arch-check
on: [push, pull_request]

jobs:
  layering:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.12'
      - name: 检查器版本漂移
        run: python3 tools/check_layering.py --version --lock tools/CHECKER.lock
      - name: 分层检查
        run: python3 tools/check_layering.py .
```

两条命令的退出码就是门槛：

- `--version --lock`：文件头与锁文件任一不一致返回 `1`（升级漏改、手工改过文件都会被抓到）；
- 正常扫描：有 ERROR 返回 `1`，用法错误返回 `2`。

`--version` 单独使用时只打印：

```
checker-version: 3.2.0
source-repo: https://github.com/BiscuiTs2026/firmware-four-layer-architecture
source-commit: self
```

---

## 3. 用 `.arch-roles` 描述非四层结构

检查器内建六个角色：`bsp` / `osal` / `components` / `app` / `glue` / `common`。
工程层数与名字不同时，不要求改目录、也不要求改名，用清单里的三条指令对齐。

### 3.1 三条指令

```
alias <层名> = <内建层>     把自己的层名映射到内建层，映射后按内建层的深度参与判定
layers = <层名>, ...        声明工程实际使用的层；未声明的层不参与判定
ignore = <相对路径/模式>     跳过厂商 SDK、生成目录、单测桩；可写多行，支持 shell 通配
```

- 指令写在清单顶部；`alias` 必须在使用它的 `layers` 与映射行之前出现。
- `alias` 目标只能是内建角色，且不支持链式别名（`alias a = b` 里的 `b` 必须是内建层）。
- `layers` 省略时按“六层全在”判定，即原有四层工程的行为不变。
- `ignore` 的路径相对工程根；`third_party/`、`build-*/` 这类目录写目录名或前缀都行。

### 3.2 两层工程的最小示例

以“业务层 `app` + 硬件层 `hal`”为例（完整可编译示例见 [`examples/two-layer/`](../examples/two-layer/)）：

```
# .arch-roles
alias hal = bsp
layers = app, hal

src/main.c       = glue
src/app_logic.*  = app
src/hal_gpio.*   = hal
```

```
src/main.c        glue   初始化与调度
src/app_logic.c   app    业务逻辑，可直接调 hal
src/hal_gpio.c    hal    GPIO 细节，只暴露 hal_ 前缀接口
third_party/      厂商包，ignore 掉
```

### 3.3 判定口径

| 情形 | 两层工程下的判定 | 依据 |
|---|---|---|
| `app` 调 `hal_*`（= `bsp_*`） | **放行** | 没有组件层可经，这是唯一向下路径 |
| `hal` 调 `app`（调用 / 本地 `extern` / include） | ERROR | 反向依赖，与四层同一口径 |
| `app` 里出现寄存器、`HAL_`、LL、厂商类型 | ERROR | 分层的最小含义，与层数无关 |
| `hal` 头文件把 `GPIO_TypeDef` 暴露给上层 | ERROR | 厂商类型不得出 BSP |
| `main.c` | 仍按 `glue`：可碰寄存器与 HAL，≤ 200 行，不承载业务 | 入口胶水层与层数无关 |
| 某个文件解析出的层不在 `layers` 里 | ERROR | 清单与代码脱节，不能靠声明把第三层藏起来 |
| 只有 `app/` 一个目录、代码直接写寄存器 | 不是两层，是没分层：这些文件按 `app` 判定，会报越层 | 层数少不等于可以不分层 |
| 厂商目录没有 `ignore`，直接映射成 `bsp` | 能跑，但每次都要解析整套厂商包（实测慢约 100 倍） | 见第 5 节实测 |

`glue` 与 `common` 始终可用，不必写进 `layers`。

**消息里的层名是内建名**：`alias hal = bsp` 之后，反向依赖报的是 `bsp/`，不是 `hal/`。
这是有意的——同一条规则在所有工程里输出同一套词，便于上游收集误报。

### 3.4 常见写错

| 写法 | 结果 | 正确做法 |
|---|---|---|
| `src/hal_gpio.c = hal` 但没写 `alias hal = bsp` | 启动即报“未知角色 hal” | 先写 `alias` |
| `layers = app, hal` 但工程里还有 `src/osal/` | ERROR：角色 osal 不在 layers 声明内 | 补进 `layers`，或先删掉该层 |
| 把 `third_party/` 映射成 `bsp` | 厂商代码里的宏、`extern`、上层符号会大量误报 | 用 `ignore` |
| 两层工程里 `main.c` 写业务 | 超 200 行即 ERROR | 业务下沉到 `app` |

---

## 4. 回灌流程（使用方 → 上游）

误报和漏报不是“使用者自己注释掉”，而是回到上游修规则。五步走：

1. **记录现场**（使用方）：命令、`.arch-roles`、触发规则的最小文件、完整输出、检查器版本与 `source-commit`。
   这些一起贴进上游 issue；缺了最小文件，上游无法判断是误报还是真问题。
2. **最小复现**（上游）：把最小文件放进 `tests/probe-<主题>/`，配 `expected.txt`。
   误报：期望输出里不含该条；漏报：期望输出里必须有该条。先让 `python3 tests/run_tests.py` 在现状下失败。
3. **改规则**（上游）：只改检查器与夹具，不顺带重构、不扩大范围；允许把规则放宽成 WARN，
   但要在 `docs/checker.md` 写清放宽理由。
4. **回归全绿**（上游）：`python3 tests/run_tests.py` 全部 PASS，包括示例工程的零发现项与“单文件拷出运行”项。
5. **发版并通知**（上游）：`CHECKER_VERSION` 递增，提交信息写清哪条规则变了、影响哪些工程；
   使用方按 4.2 升级。

### 4.1 新规则进 CI gate 的三条门槛

一条新规则想从“建议”升为“CI 里会拦人的 ERROR”，三条缺一不可：

1. **有最小复现夹具**：`tests/probe-*` 里不超过几十行，只含触发该规则的最小文件，并有期望输出；
2. **在至少一个真实工程上零误报**：给出命令、工程规模与扫描结果。只在自己构造的样例上通过不算，
   因为构造样例天然缺少真实工程里的厂商宏、调试桩与历史遗留写法；
3. **说清违规的实际后果**：一句话说明会导致什么可观测故障或维护代价（例如“换芯片要改上层”、
   “ISR 里阻塞把 10 ms 节拍拉长”）。说不清后果的规则，写进 `docs/` 当建议，不进 gate。

不满足三条的规则只能以 WARN 或文档建议的形式存在，且必须在 `docs/checker.md` 标注为“未达 gate 门槛”。

### 4.2 使用方升级步骤

1. 读上游提交说明，确认改的是自己会碰到的规则；
2. 覆盖 `tools/check_layering.py`，把文件头 `source-commit` 改成上游提交哈希；
3. 更新 `tools/CHECKER.lock`（版本、提交、`synced-at`），`local-patches` 保持与事实一致；
4. 本地跑一遍 `python3 tools/check_layering.py .`，对新增发现逐条判定：真问题进 backlog，
   误报按第 4 节回灌，不要把新版本直接塞进 CI 让流水线红着；
5. 提交，交由 CI 用 `--version --lock` 复核。

---

## 5. 实测：接入前后的差别

在一套真实的两层工程上（STM32F103C8T6，`third_party/` 内含 CubeF1 厂商包，约 1200 个 C 文件）用同一版本检查器对比：

| 配置 | 结果 | 耗时 |
|---|---|---|
| 不写清单，直接扫（该工程当时尚无清单） | 厂商包全部算“未分层文件”：WARN 刷屏，且每文件上限 5 条，真实数量被截断 | 17 s 量级 |
| 清单把 `third_party/*` 映射成 `bsp` | 没有误报（厂商包在最底层，不反调上层），但要逐文件解析整套厂商包 | 17.4 s |
| 清单 + `ignore = third_party/, build-*/, tests/` | 同样的 1 ERROR / 3 WARN，且该 ERROR 是真实的（`main.c` 1277 行超限） | 0.16 s |

结论：厂商包映射成 `bsp` 不是错，但**`ignore` 才是推荐做法**——省掉两个数量级的扫描时间，
也不会让厂商宏进入宏索引去参与越层判定。接入第一步先把厂商目录与构建目录挡掉，否则噪音会把真问题埋掉。

---

## 6. 已知边界

- 运行环境：Python 3.8+，仅标准库。已验证 3.12 / 3.14。
- 只做文本启发式分析，不做编译；宏展开、条件编译、函数指针流都按近似处理。
- `ignore` 只按路径，不会因为“这个文件是自动生成”而自行判断。
- 检查器不读上游仓库，所以“上游回归全绿”只保证夹具覆盖到的形态；使用方工程的新形态仍可能触发新误报，
  这正是升级步骤第 4 步要求人工过一遍的原因。
- 初始化顺序、ISR 与主循环契约、回调穿层不在任何版本的可机检范围内，见 `rules.md` 第 13 节。
