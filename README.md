# firmware-four-layer-architecture

Four-layer architecture rules that constrain AI coding agents writing MCU firmware.

Most AI-generated firmware rots the same way: a bloated `main.c`, vendor HAL calls leaking into application code, and a chip swap that forces a rewrite of everything. These rules make the layer boundaries **checkable** instead of advisory.

## What is inside

| Path | Purpose |
|---|---|
| `rules.md` | The rules as a single prompt / project rule file (118 lines) |
| `skill/` | Installable agent skill: `SKILL.md` + references + checker |
| `docs/guide-zh.md` | Design rationale, usage and acceptance tests (Chinese) |
| `examples/` | Deliberately broken and clean sample projects for the checker |

## The four layers

```
APP -> components -> OSAL -> BSP -> hardware
```

| Layer | Owns | Must never |
|---|---|---|
| APP | message subscription, flow orchestration | touch registers, HAL or RTOS API |
| components | business logic, event publishing | include each other's private headers |
| OSAL | task / delay / mutex / queue / tick abstraction | expose chip or RTOS headers |
| BSP | registers, HAL, pin mapping | carry business semantics |

## Four prohibitions

1. No reverse dependency: lower layers never include upper ones.
2. No cross-layer calls: APP never calls OSAL or BSP directly.
3. No header leakage: no `<stm32f1xx_hal.h>`, no `FreeRTOS.h` in public headers.
4. No **type** leakage: vendor types (`SPI_HandleTypeDef`, `GPIO_TypeDef`, `nrfx_spi_t`) must not appear in any signature visible outside `bsp/`. Banning `#include` can be bypassed by inlining a definition; banning the type cannot.

Section 1.1 covers generated projects: write only inside `USER CODE` regions, never touch `.ioc` / `.syscfg` / SDK-generated files, deliver one verifiable change per round, show a diff before applying it.

## Checker

```bash
python3 skill/scripts/check_layering.py <project-root>
```

Detects five classes of problems: cross-layer HAL or register access, platform-header leakage, vendor-type leakage, module-crossing globals, and `if-else` nesting deeper than 3.

Verified against the bundled samples: the broken project reports 4 errors plus 2 warnings; the clean project reports none. The checker is heuristic, so add `// arch-allow` to a line you have confirmed is a false positive.

## Using the rules

- **As a prompt**: paste `rules.md` into your assistant, or save it as `CLAUDE.md` / `.cursor/rules/`.
- **As a skill**: install the `skill/` folder into your agent's skills directory.
- **Conflict policy**: a user instruction always wins, but the agent must state the conflict and the resulting technical debt up front.

## Why not just MISRA C or BARR-C

Those standards constrain the language, not the layer boundaries: they say nothing about who may call whom. Zephyr's device model is the industrial-grade version of a correct BSP, but it is an entire operating system. This sits in between: small enough for a course project, strict enough to survive a chip swap.

## Scope and limits

C99, STM32 / GD32 class MCUs, bare-metal or RTOS. Not covered: RTOS selection, circuit design, peripheral timing, C++ projects, performance tuning.

## License

MIT. See [LICENSE](LICENSE).
