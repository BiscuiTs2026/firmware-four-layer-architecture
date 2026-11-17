# 合规示例（扁平 src/ + 角色清单）

不拆目录的老工程也能被分层检查覆盖：用 `.arch-roles` 声明每个文件属于哪一层。

```
src/main.c         -> glue（入口胶水）
src/i2c_soft.c/.h  -> bsp（寄存器与引脚映射）
src/telemetry.c/.h -> components（PID 与 CSV 组帧，不碰寄存器）
src/board_config.h -> bsp（板级常量）
src/types.h        -> common（err_t）
```

自检：

```bash
python3 ../../skill/scripts/check_layering.py .
# 期望：0 ERROR / 0 WARN，并提示"使用角色清单"
```

这套写法的价值：先把现有工程纳入约束、看清违规在哪里，再决定要不要真的拆目录。
