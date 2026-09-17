cmake_minimum_required(VERSION 3.16)
project(ci_demo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# 1) 纯逻辑库：不依赖任何硬件
#    这个 target 就是你将来要"抽出来"的那部分代码。
#    PUBLIC 关键字表示：谁链接我，谁就自动获得 include 目录。
# ---------------------------------------------------------------------------
add_library(adc_filter STATIC src/adc_filter.c)
target_include_directories(adc_filter PUBLIC include)
target_compile_options(adc_filter PRIVATE -Wall -Wextra -Werror)

# ---------------------------------------------------------------------------
# 2) PC 版单元测试
# ---------------------------------------------------------------------------
enable_testing()

add_executable(test_adc_filter tests/test_adc_filter.c)
target_link_libraries(test_adc_filter PRIVATE adc_filter)
target_compile_options(test_adc_filter PRIVATE -Wall -Wextra -Werror)

add_test(NAME adc_filter COMMAND test_adc_filter)

# ---------------------------------------------------------------------------
# 3) 将来接交叉编译时，在这里挂真实的固件 target，例如：
#
#    add_executable(firmware src/main.c src/hal_stm32.c)
#    target_link_libraries(firmware PRIVATE adc_filter)
#
#    然后用 -DCMAKE_TOOLCHAIN_FILE=arm-gcc.cmake 走交叉编译。
#    adc_filter 这个库两边共用 —— 这就是解耦带来的复用。
# ---------------------------------------------------------------------------
