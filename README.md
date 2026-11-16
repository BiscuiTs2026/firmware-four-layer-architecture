# firmware-four-layer-architecture

Four-layer architecture rules that constrain AI coding agents writing MCU firmware.

Most AI-generated firmware rots the same way: a bloated `main.c`, vendor HAL calls leaking into application code, and a chip swap that forces a rewrite of everything. These rules turn the layer boundaries into constraints an agent has to satisfy and a script can partly check.

> **Status**: the rules have **not** yet been validated against a live AI session. What *is* verified today: the checker passes its regression suite (`tests/probe`, 14 planted violations, all reported, no false positives), and the clean sample reports zero findings. Treat the rule text as unproven until it survives a real session.

## What is inside

| Path | Purpose |
|---|---|
| `rules.md` | The rules as a single prompt / project rule file (130 lines) |
| `skill/` | Installable agent skill: `SKILL.md`, full reference, checker |
| `docs/guide-zh.md` | Design rationale, usage and acceptance tests (Chinese) |
| `examples/` | `bad/` deliberately broken, `good/` a clean mini project |
| `tests/probe/` | Regression fixture: 14 planted violations and expected findings |

## The four layers

```
APP -> components -> OSAL -> BSP -> hardware
```

| Layer | Owns | Must never |
|---|---|---|
| APP | message subscription, flow orchestration | touch registers, HAL, RTOS API or BSP |
| components | business logic, event publishing | include each other's private headers |
| OSAL | task / delay / mutex / queue / tick abstraction | expose chip or RTOS headers |
| BSP | registers, HAL, pin mapping | carry business semantics |

`common/` holds pure types (`err_t`, `message_t`, `msg_id` table). Any layer may include it; it may include no layer. It sits outside the dependency order.

## Four prohibitions

1. No reverse dependency: lower layers never include upper ones, and `common/` includes nothing.
2. No cross-layer calls: APP never calls BSP directly. APP may use only OSAL time primitives (`osal_sleep_ms`, `osal_tick_ms`), never queues, mutexes or task creation.
3. No header leakage: no `<stm32f1xx_hal.h>`, `FreeRTOS.h`, `task.h` or `gd32f10x.h` in any `.c` outside `bsp/`, and none in any public header.
4. No **type** leakage: vendor types (`SPI_HandleTypeDef`, `GPIO_TypeDef`, `nrfx_spi_t`) must not appear in any signature visible outside `bsp/`. Banning `#include` can be bypassed by inlining a definition; banning the type cannot.

Section 1.1 covers generated projects: write only inside `USER CODE` regions, never touch `.ioc` / `.syscfg` / SDK-generated files, deliver one verifiable change per round, show a diff before applying it.

## Checker

```bash
python3 skill/scripts/check_layering.py <project-root>
```

Nine checks: HAL / LL / SPL / RTOS calls and register access outside `bsp/`; platform headers in `.c` and public `.h`; vendor types in headers; reverse `#include`; module-crossing globals; `extern` variables; `if` nesting deeper than 3; unlayered files (WARN); APP calling BSP or using OSAL beyond the time primitives.

### Regression

```bash
python3 skill/scripts/check_layering.py tests/probe --max-per-file 2
```

The probe plants 14 violations — FreeRTOS and GD32 headers, `__HAL_RCC_*`, the LL library, GD32 SPL calls, `vTaskDelay`, struct and array globals, `extern`, a reverse include, HAL plus registers in `main.c`, digit-suffixed registers. Expected: **11 errors + 3 warnings, no false positives**. A `HAL_GPIO_WritePin` that appears only inside a block comment must stay silent.

The checker is heuristic; add `// arch-allow` to a line you have confirmed is a false positive. Files outside `app/ components/ osal/ bsp/` cannot be judged — they are reported as WARN so the blind spot stays visible instead of silent.

## Using the rules

- **As a prompt**: paste `rules.md` into your assistant, or save it as `CLAUDE.md` / `.cursor/rules/`.
- **As a skill**: install the `skill/` folder into your agent's skills directory.
- **Conflict policy**: a user instruction always wins, but the agent must state the conflict and the resulting technical debt up front, and log it in `TECH_DEBT.md`.

## Why not just MISRA C or BARR-C

Those standards constrain the language, not the layer boundaries: they say nothing about who may call whom. Zephyr's device model is the industrial-grade version of a correct BSP, but it is an entire operating system. This sits in between: small enough for a course project, strict enough to survive a chip swap.

## Scope and limits

C99, STM32 / GD32 class MCUs, bare-metal or RTOS. Not covered: RTOS selection, circuit design, peripheral timing, C++ projects, performance tuning.

## License

MIT. See [LICENSE](LICENSE).
