cmake_minimum_required(VERSION 3.20)

# 示例只编译成静态库，不链接也不运行。用 arm-none-eabi-gcc 做主机侧语法检查时，
# CMake 默认的“链接可执行文件”探测会因为缺主机 CRT 与 --major-image-version 失败，
# 这里让编译器探测只产出静态库。对宿主 gcc/Clang 无副作用。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(firmware_four_layer_architecture LANGUAGES C)

# 主机侧语法检查：用 tests/host-stubs 里的最小 CMSIS/HAL 头，把 examples/ 编译成静态库。
# 它验证接口与 C 语法，不验证寄存器行为，也不是目标板构建。
# 目标板构建需要 STM32CubeF1 + arm-none-eabi-gcc，见 README「示例可构建到什么程度」。
option(FOUR_LAYER_HOST_CHECK "Build examples against host stubs" ON)

if(FOUR_LAYER_HOST_CHECK)
  enable_testing()
  add_subdirectory(tests/host-build)
endif()
