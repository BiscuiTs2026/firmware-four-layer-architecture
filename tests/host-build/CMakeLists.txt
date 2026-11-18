# 主机侧编译示例：只验证 C 语法、头文件路径与接口一致性。
# 产物是静态库（不链接、不运行），因为外设行为依赖真实芯片。

add_library(host_stubs INTERFACE)
target_include_directories(host_stubs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../host-stubs)
target_compile_features(host_stubs INTERFACE c_std_99)

if(MSVC)
  target_compile_options(host_stubs INTERFACE /utf-8 /W3)
else()
  target_compile_options(host_stubs INTERFACE -Wall -Wextra)
endif()

set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../examples)

# good-hal：HAL 形态
add_library(example_good_hal STATIC
  ${EXAMPLES_DIR}/good-hal/main.c
  ${EXAMPLES_DIR}/good-hal/app/app.c
  ${EXAMPLES_DIR}/good-hal/components/comp_blink.c
  ${EXAMPLES_DIR}/good-hal/osal/osal.c
  ${EXAMPLES_DIR}/good-hal/bsp/bsp_led.c
)
target_link_libraries(example_good_hal PRIVATE host_stubs)
target_include_directories(example_good_hal PRIVATE
  ${EXAMPLES_DIR}/good-hal
  ${EXAMPLES_DIR}/good-hal/app
  ${EXAMPLES_DIR}/good-hal/components
  ${EXAMPLES_DIR}/good-hal/osal
  ${EXAMPLES_DIR}/good-hal/bsp
  ${EXAMPLES_DIR}/good-hal/common
)

# good-baremetal：寄存器形态 + 固定地址快照
add_library(example_good_baremetal STATIC
  ${EXAMPLES_DIR}/good-baremetal/main.c
  ${EXAMPLES_DIR}/good-baremetal/app/app.c
  ${EXAMPLES_DIR}/good-baremetal/components/comp_telemetry.c
  ${EXAMPLES_DIR}/good-baremetal/osal/osal_tick.c
  ${EXAMPLES_DIR}/good-baremetal/bsp/bsp_gpio.c
)
target_link_libraries(example_good_baremetal PRIVATE host_stubs)
target_include_directories(example_good_baremetal PRIVATE
  ${EXAMPLES_DIR}/good-baremetal
  ${EXAMPLES_DIR}/good-baremetal/app
  ${EXAMPLES_DIR}/good-baremetal/components
  ${EXAMPLES_DIR}/good-baremetal/osal
  ${EXAMPLES_DIR}/good-baremetal/bsp
  ${EXAMPLES_DIR}/good-baremetal/common
)

# flat：扁平 src/ + 角色清单
add_library(example_flat STATIC
  ${EXAMPLES_DIR}/flat/src/main.c
  ${EXAMPLES_DIR}/flat/src/bsp_time.c
  ${EXAMPLES_DIR}/flat/src/i2c_soft.c
  ${EXAMPLES_DIR}/flat/src/timebase.c
  ${EXAMPLES_DIR}/flat/src/telemetry.c
)
target_link_libraries(example_flat PRIVATE host_stubs)
target_include_directories(example_flat PRIVATE
  ${EXAMPLES_DIR}/flat/src
)

# two-layer：两层工程（app + hal）+ alias / layers 指令
add_library(example_two_layer STATIC
  ${EXAMPLES_DIR}/two-layer/src/main.c
  ${EXAMPLES_DIR}/two-layer/src/app_logic.c
  ${EXAMPLES_DIR}/two-layer/src/hal_gpio.c
)
target_link_libraries(example_two_layer PRIVATE host_stubs)
target_include_directories(example_two_layer PRIVATE
  ${EXAMPLES_DIR}/two-layer/src
)
