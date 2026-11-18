# 两层工程示例（app + hal）

四层规范的最小可运行子集：只有业务层与硬件层，没有 OSAL 和组件层。
用 `.arch-roles` 把 `hal` 映射到内建角色 `bsp`，并声明 `layers = app, hal`。

```
src/main.c        glue   初始化与调度
src/app_logic.c   app    业务逻辑，允许直接调 hal
src/hal_gpio.c    hal    GPIO 细节，只暴露 hal_ 前缀接口
```

关键判定口径：

- `app` 仍然不得出现寄存器、HAL、LL、厂商类型；这些只能在 `hal`（= `bsp`）里。
- `app` 直调 `hal_*` **不算违规**——两层工程没有组件层可经，这是唯一向下路径。
- `hal` 反调 `app` 仍是 ERROR（反向依赖），消息里显示的是内建层名 `bsp`。
- `main.c` 仍按 `glue` 判定：可碰寄存器和 HAL，但 ≤ 200 行、不承载业务。

```bash
python3 ../../skill/scripts/check_layering.py .
```

接入其它工程的完整说明见 [docs/integration.md](../../docs/integration.md)。
