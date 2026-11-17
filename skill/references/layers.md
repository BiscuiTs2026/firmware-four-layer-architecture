# 分层速查

完整规则见仓库根目录 `rules.md`。本文件只做写代码时的速查，不重复规则条文。

## 接口形态

| 层 | 前缀 | 典型签名 | 返回 |
|---|---|---|---|
| BSP | `bsp_` | `err_t bsp_<对象>_<动作>(句柄, 参数)` | `err_t` |
| OSAL | `osal_` | `uint32_t osal_tick_ms(void)` | 值或 `err_t` |
| 组件 | `comp_` | `err_t comp_<功能>_init/start/stop(void)` | `err_t` |
| APP | `app_` | `void app_<流程>_run(void)` | 无或 `err_t` |

## 参数传递

| 从 → 到 | 允许 | 禁止 |
|---|---|---|
| APP → 组件 | 标准整型、自定义枚举、组件句柄 | 厂商类型、BSP 句柄 |
| 组件 → OSAL | 时基、延时、队列句柄 | RTOS 原生类型 |
| 组件 → BSP | 语义化 ID（`io_id_t`、`uart_id_t`） | `GPIO_TypeDef*`、`UART_HandleTypeDef*` |
| 任意层 → 快照 | 只读 | 写入非本模块归属的快照 |

## 文件归属判断

- 出现 `RCC->`、`GPIOA->`、`HAL_`、`LL_`、`gpio_`（GD32 SPL）→ 必须是 BSP 或胶水层。
- 出现产品概念（“调参”“心跳”“遥测”“配网”）→ 不能是 BSP。
- 出现 `while` + `HAL_GetTick()` 等超时轮询 → 不能是 APP，下沉到组件或 BSP。
- 只有初始化、中断分发、主循环调度 → 胶水层。

## 新增模块检查单

- [ ] 目录与角色清单都登记了
- [ ] 头文件不含芯片头 / RTOS 头 / 厂商类型
- [ ] 内部状态全 `static`
- [ ] 生命周期函数成对出现（init / start / stop）
- [ ] 失败路径返回 `err_t`，不静默
- [ ] 有对应的检查器扫描结果（无 ERROR）
