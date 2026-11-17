---
name: firmware-four-layer-architecture
display_name: "固件四层架构规范"
description: 按 BSP / OSAL / 组件 / APP 四层生成或审查 MCU 固件。适用于：新建或重构 STM32、GD32 工程；为嵌入式工程新增模块；评审越层调用、跨模块全局变量、深层嵌套、ISR 阻塞、固定地址段冲突；判断"换芯片要改哪些层"；约束 CubeMX / SysConfig 生成工程里 AI 的改动边界。不适用于纯硬件电路设计、RTOS 选型决策和 C++ 工程。
---

# 固件四层架构规范

把代码约束在 BSP / OSAL / 组件 / APP 四层之内，让换芯片、加功能、换人接手都不牵动全局。

## 使用顺序

1. 读 `../rules.md`，拿到完整约束。它同时支持 HAL 形态与裸机形态，差异在规则第 2 节。
2. 判断工程形态：
   - 有 `.ioc` / `Drivers/` / `Core/Inc/*_hal*.h` → HAL 形态，只改 `USER CODE` 区；
   - 无上述文件、有自写启动文件与链接脚本 → 裸机形态。
3. 判断判层方式：有 `app/ bsp/ osal/ components/` 目录就直接用；扁平 `src/` 先写 `.arch-roles`。
4. 按约束写或改代码。
5. 跑 `scripts/check_layering.py <工程根目录>`，修到无 ERROR 且无未豁免 WARN。
6. 按交付合同输出：改动清单、依赖方向、接口变更、时序影响、自检表、验证建议、技术债。

## 八条铁律

1. 依赖只向下：`APP → 组件 → OSAL → BSP`，下层不得 `#include` 上层。
2. APP 不碰寄存器、HAL、RTOS API，不直调 BSP；延时只走 `osal_sleep_ms`。
3. 公共头不得出现芯片头、RTOS 头、厂商类型（`SPI_HandleTypeDef`、`GPIO_TypeDef` 等）。
4. 跨模块写只走参数中心或公共 API；禁止 `extern` 传数据，只读快照除外。
5. 入口胶水层（`main.c` 等）只做初始化与中断分发，≤ 200 行代码。
6. `if-else` 嵌套不超过 3 层，超出改用表驱动或状态机。
7. ISR 内只置标志、发消息、清中断；禁止延时、打印、动态内存、阻塞锁。
8. 慢外设分块非阻塞；固定地址段不得重叠，禁止通配段吞并。

## 常见误判

- 字模 / 查表渲染的 `for → for → if` 会被算作深层嵌套，用 `// arch-allow: 字模循环` 标注后放行；
- `SystemCoreClock` 等 CMSIS 必需符号已默认豁免；
- 带 `__attribute__((section(...)))` 的快照变量不按跨模块全局变量处理。

## 不要做的事

- 不要为了跑通检查器而大规模移动文件；先写 `.arch-roles`，再按修改批次下沉。
- 不要把业务逻辑写进 `while (1)` 里凑功能。
- 不要为了让检查器变绿而给行加 `arch-allow`；一个文件超过三处豁免，说明结构该改了。
