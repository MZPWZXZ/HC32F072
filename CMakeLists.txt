# ----------------------------------------------------------------------------
# HC32F072KA 固件工程顶层构建脚本
#
# 依赖:CMake >= 3.20 + arm-none-eabi-gcc(见 cmake/arm-none-eabi-gcc.cmake)
# 产物:build/hc32f072ka.{elf,hex,bin,map}
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(hc32f072ka
        VERSION 0.1.0
        DESCRIPTION "HC32F072KA firmware demo, internal RC clock 48MHz"
        LANGUAGES C ASM)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "请使用交叉工具链配置,例如:\n"
            "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake")
endif()

set(TARGET hc32f072ka)

# ---- 顶层目录 ----
set(SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR})
set(DDL_DIR     ${SOURCE_DIR}/third_party/hc32f072_ddl)
set(CMSIS_DIR   ${SOURCE_DIR}/third_party/cmsis)

# ---- CPU/通用编译选项(C 与汇编都生效) ----
add_compile_options(
    -mcpu=cortex-m0plus
    -mthumb
    -ffunction-sections
    -fdata-sections
)

# ---- 公共头文件搜索路径 ----
set(INCLUDE_DIRS
    ${SOURCE_DIR}/config                # ddl_device.h
    ${SOURCE_DIR}/src                   # 应用头文件
    ${SOURCE_DIR}/src/bsp               # board.h / bsp_*.h
    ${DDL_DIR}/mcu/common               # HC32F072.h system_*.h ...
    ${DDL_DIR}/driver/inc               # ddl.h sysctrl.h gpio.h uart.h flash.h
    ${CMSIS_DIR}                        # core_cm0plus.h(cmsis_gcc.h ...)
)

# ============================================================================
# 1. 第三方 DDL 静态库(只读代码,编译时屏蔽警告)
# ============================================================================
set(DDL_SOURCES
    ${DDL_DIR}/driver/src/ddl.c
    ${DDL_DIR}/driver/src/sysctrl.c
    ${DDL_DIR}/driver/src/gpio.c
    ${DDL_DIR}/driver/src/uart.c
    ${DDL_DIR}/driver/src/flash.c
    ${DDL_DIR}/mcu/common/system_hc32f072.c
    ${DDL_DIR}/mcu/common/interrupts_hc32f072.c
)

add_library(hc32f072_ddl STATIC ${DDL_SOURCES})
target_include_directories(hc32f072_ddl PRIVATE ${INCLUDE_DIRS})
# 第三方代码保持原样编译,不做警告级审查(警告属于厂商代码风格问题)
target_compile_options(hc32f072_ddl PRIVATE -w)

# ============================================================================
# 2. 应用代码(严格警告审查:-Wall -Wextra -Werror)
# ============================================================================
set(APP_SOURCES
    ${SOURCE_DIR}/src/main.c
    ${SOURCE_DIR}/src/bsp/bsp_sysclk.c
    ${SOURCE_DIR}/src/bsp/bsp_led.c
    ${SOURCE_DIR}/src/bsp/bsp_uart.c
)

add_library(hc32f072ka_app STATIC ${APP_SOURCES})
target_include_directories(hc32f072ka_app PRIVATE ${INCLUDE_DIRS})
target_compile_options(hc32f072ka_app PRIVATE -Wall -Wextra -Werror)

# ============================================================================
# 3. 固件可执行文件(启动汇编 + 链接脚本)
# ============================================================================
set(LINKER_SCRIPT ${SOURCE_DIR}/startup/hc32f072ka_flash.ld)

add_executable(${TARGET}.elf
    ${SOURCE_DIR}/startup/startup_hc32f072.S
    ${SOURCE_DIR}/src/syscalls.c   # newlib 系统调用桩(直接编入,确保先于库解析)
)

target_include_directories(${TARGET}.elf PRIVATE ${INCLUDE_DIRS})
target_link_libraries(${TARGET}.elf
    PRIVATE
        hc32f072ka_app
        hc32f072_ddl
)

target_link_options(${TARGET}.elf PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    --specs=nano.specs          # 精简 C 库(系统调用桩见 src/syscalls.c)
    -Wl,--gc-sections           # 裁剪未引用函数/数据
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET}.map
    -T${LINKER_SCRIPT}
)

# ---- 后处理:hex / bin / 内存占用打印 ----
set(ELF_FILE $<TARGET_FILE:${TARGET}.elf>)
set(HEX_FILE ${CMAKE_BINARY_DIR}/${TARGET}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${TARGET}.bin)

add_custom_command(TARGET ${TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   ${ELF_FILE} ${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${BIN_FILE}
    COMMAND ${CMAKE_SIZE} ${ELF_FILE}
    COMMENT "生成 ${TARGET}.hex / ${TARGET}.bin,并打印内存占用"
)

# 便捷别名:cmake --build build --target firmware
add_custom_target(firmware DEPENDS ${TARGET}.elf)
