# 实测数据

记录本仓库在真实工程上的表现，用于判断它到底值不值得装。

## 测试对象

作者手上一套裸机工程（STM32F103C8T6，无 HAL，GCC + CMake + Ninja）：

| 工程 | 代码行数 | Flash | 说明 |
|---|---|---|---|
| A | 3569 | 6.6 KB | OLED + 蓝牙 + 20 Hz 遥测 + PID 调参 |
| B | 约 330 | 约 3 KB | 上板时序自检，单文件 `main.c` |
| C | 约 200 | 约 2 KB | 板级连通性检查，单文件 `main.c` |

## 结果一：不配置直接扫

```
python3 check_layering.py A
→ 0 ERROR / 31 WARN（全部是"未分层文件"）
```

原因：所有代码在扁平 `src/`，没有 `app/ bsp/ osal/ components/`，也没有角色清单。
**检查器一条分层规则都没执行，却返回退出码 0。** 这是最需要防的失败模式：绿灯不等于合规。

## 结果二：写角色清单后扫

```ini
src/main*.c      = glue
src/i2c_soft.*   = bsp
src/ssd1306.*    = bsp
src/bt_link.*    = bsp
src/gfx.*        = components
src/fonts.*      = components
src/telemetry.*  = components
src/dbg_console.*= app
src/baud_cfg.*   = app
```

```
→ 37 ERROR
```

| 类别 | 条数 | 判断 |
|---|---|---|
| 越层访问寄存器 | 25 | 真问题：波特率重配、PA11 采样、DWT 计时都写在组件/应用层 |
| `extern` 跨模块变量 `g_ms` | 3 | 真问题：BSP 与组件反向依赖入口层的时基 |
| 平台头泄漏 | 2 | 真问题：组件层 include `stm32f1xx.h` |
| 跨模块全局变量 `g_demo_*` | 4 | 真问题但改动极小：改 `static` 后地址仍可在 map 里查到 |
| `if` 嵌套超 3 层 | 3 | 误报：2 处 16×16 字模双重循环，1 处 `switch` 状态机 |

**可执行率 34/37 ≈ 92%。**

## 结果三：大 `main.c`

| 工程 | `main.c` 代码行 | 结果 |
|---|---|---|
| B | 268 | 1 ERROR：胶水层超限 |
| C | 155 | 0 ERROR（上限 200，属合理范围） |

胶水层行数上限取 200 的理由：C 这类顺序执行的板级自检，155 行是正常体量；B 的 268 行里已经混入了协议解析与时序状态机，确实是该下沉的信号。

## 结论

1. 检查器有价值，但**前提是先把文件映射到层**。目录分层或角色清单，二选一。
2. 误报集中在“结构嵌套”这一项，其余 11 项在真实代码上表现稳定。
3. 对 6 KB 级工程，改造顺序建议：先修 `extern` 时基 → 把寄存器操作收进 BSP → 最后才拆 `telemetry` 这类万能文件。反过来做会先卡在最大的一块上。

## 复现

```bash
git clone <本仓库>
cd firmware-four-layer-architecture
python3 tests/run_tests.py          # 回归夹具，应全 PASS
python3 skill/scripts/check_layering.py <你的工程>
```
