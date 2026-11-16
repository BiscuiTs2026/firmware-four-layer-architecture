---
name: firmware-four-layer-architecture
display_name: "四层架构规范"
description: 按 BSP / OSAL / 组件 / APP 四层架构生成或审查单片机 C 代码。适用于：新建或重构 STM32、GD32 等固件工程结构；为嵌入式工程新增功能模块；评审或排查越层调用、跨模块全局变量、深层 if-else 嵌套；判断"换芯片需要改哪些层"；审查 CubeMX / SysConfig 生成工程里 AI 的改动边界。不适用于纯硬件电路设计、RTOS 选型决策和 C++ 工程。
---

# 四层架构规范

把代码约束在 BSP / OSAL / 组件 / APP 四层之内，让换芯片、加功能、换人接手都不牵动全局。

## 使用顺序

1. 读 `references/layers.md`，拿到完整约束：分层表、四条越层禁令、工程修改边界、数据流、参数中心、鲁棒性、交付契约、自检清单。
2. 按约束写或改代码。
3. 跑 `scripts/check_layering.py <工程根目录>` 自检，修到没有 ERROR。
4. 按交付契约输出五项：改动清单、依赖方向说明、接口变更、自检表、验证建议。

## 八条铁律（始终生效）

1. 依赖只向下：`APP → 组件 → OSAL → BSP`，下层不得 `#include` 上层。
2. APP 不碰寄存器、HAL、RTOS API，只订阅消息、编排流程。
3. 公共头不得出现芯片头、RTOS 头、厂商类型（`SPI_HandleTypeDef`、`GPIO_TypeDef` 等）。
4. 跨模块传数据只走消息总线，不新增跨模块全局变量。
5. `if-else` 嵌套不超过 3 层，超出改用表驱动状态机或事件驱动。
6. ISR 只允许置标志、发消息、清中断标志。
7. 对外 API 判空 + 判范围，统一返回 `err_t`；可调参数集中到参数中心。
8. CubeMX / SysConfig 工程只改 `USER CODE` 区，不动 `.ioc`、`.syscfg` 与 SDK 生成文件。

## 脚本

```bash
python3 scripts/check_layering.py <工程根目录>
```

检查五项：越层 HAL / 寄存器调用、平台头泄漏、厂商类型泄漏、跨模块全局变量、`if-else` 嵌套深度。输出 `ERROR` / `WARN`，退出码非 0 表示存在 ERROR。

脚本是启发式静态检查，可能出现误报；确认为误报时在该行加 `// arch-allow` 豁免。

## 冲突处理

用户当次指令与本规范冲突时，以用户指令为准，但必须在回答开头指出冲突点及由此产生的技术债，不得默默服从。
