cmake_minimum_required(VERSION 3.20)

project(firmware_four_layer_architecture LANGUAGES C)

# 主机侧语法检查：用 tests/host-stubs 里的最小 CMSIS/HAL 头，把 examples/ 编译成静态库。
# 它验证接口与 C 语法，不验证寄存器行为，也不是目标板构建。
# 目标板构建需要 STM32CubeF1 + arm-none-eabi-gcc，见 README「示例可构建到什么程度」。
option(FOUR_LAYER_HOST_CHECK "Build examples against host stubs" ON)

if(FOUR_LAYER_HOST_CHECK)
  enable_testing()
  add_subdirectory(tests/host-build)
endif()
