# examples/bad 违规样例

每条规则至少踩一次，用于演示检查器能抓到什么、报告长什么样。

```bash
python3 ../../skill/scripts/check_layering.py .
# 期望：15 ERROR / 1 WARN，退出码 1
```

| 文件 | 植入的违规 |
|---|---|
| `src/main.c` | 被角色清单错误地判成 `app`，于是平台头与寄存器访问全部报错（对照：写 `glue` 就不报） |
| `src/app_main.c` | 平台头泄漏、跨模块全局、APP 直调 BSP、`HAL_Delay` 阻塞、决策型 `if` 嵌套 4 层 |
| `src/comp_sensor.c` | 组件层引平台头、越层调 HAL、跨模块全局、`extern` 变量、ISR 内 `printf`、`.ghost_ram` 段未在链接脚本定义 |
| `ld/app.ld` | `.snap_ram` 与 `.log_ram` 同址 0x20003800 |
| `src/bsp_io.c` | 无（BSP 写寄存器合法，作为对照） |

修复方向：`main.c` 的角色改 `glue`；APP 只调组件 API；组件层去掉芯片头；状态机替掉深层决策嵌套；
ISR 只置标志；观测段在 `.ld` 里定址、标 `(NOLOAD)`、地址不重叠。
