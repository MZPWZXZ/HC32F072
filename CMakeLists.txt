# ----------------------------------------------------------------------------
# HC32F072KA 固件工程顶层构建脚本(Bootloader 分支:双镜像)
#
#   hc32f072ka_boot : Bootloader  @0x00000000(32KB,串口 IAP)
#   hc32f072ka_app  : App        @0x00008000(96KB,由 Bootloader 升级)
#
# 依赖:CMake >= 3.20 + arm-none-eabi-gcc(见 cmake/arm-none-eabi-gcc.cmake)
# 产物:build/*.elf/.map;dist/*.hex/.bin(boot 与 app)
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(hc32f072ka
        VERSION 0.1.0
        DESCRIPTION "HC32F072KA firmware: bootloader + app (IAP over UART)"
        LANGUAGES C ASM)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "请使用交叉工具链配置,例如:\n"
            "  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake")
endif()

# ---- 顶层目录 ----
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDL_DIR    ${SOURCE_DIR}/third_party/hc32f072_ddl)
set(CMSIS_DIR  ${SOURCE_DIR}/third_party/cmsis)

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
    ${SOURCE_DIR}/src                   # 应用头文件 / iap_shared.h
    ${SOURCE_DIR}/src/bsp               # board.h / bsp_*.h
    ${SOURCE_DIR}/bootloader            # IAP 协议模块头
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
# 第三方代码保持原样编译,不做警告级审查
target_compile_options(hc32f072_ddl PRIVATE -w)

# ============================================================================
# 2. 核心库(BSP),boot 与 app 共用,严格警告
#    (newlib 系统调用桩 syscalls.c 不放入静态库:直接编入各可执行文件,
#     避免静态库在 libc 之前被扫描导致 _sbrk 等符号解析不到的链接顺序问题)
# ============================================================================
set(CORE_SOURCES
    ${SOURCE_DIR}/src/bsp/bsp_sysclk.c
    ${SOURCE_DIR}/src/bsp/bsp_led.c
    ${SOURCE_DIR}/src/bsp/bsp_uart.c
)

add_library(hc32f072_core STATIC ${CORE_SOURCES})
target_include_directories(hc32f072_core PRIVATE ${INCLUDE_DIRS})
target_compile_options(hc32f072_core PRIVATE -Wall -Wextra -Werror)

# ============================================================================
# 3. 固件镜像生成函数(双目标共用)
# ============================================================================
set(DIST_DIR ${SOURCE_DIR}/dist)   # 烧录文件输出目录(与 build 同级)

function(hc32f072_add_firmware TARGET_NAME LD_SCRIPT)
    add_executable(${TARGET_NAME}.elf
        ${SOURCE_DIR}/startup/startup_hc32f072.S
        ${ARGN}                     # 各镜像独有源码(如 main.c)
    )
    target_include_directories(${TARGET_NAME}.elf PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME}.elf
        PRIVATE hc32f072_core hc32f072_ddl)

    # 自研代码严格警告审查
    target_compile_options(${TARGET_NAME}.elf PRIVATE -Wall -Wextra -Werror)

    target_link_options(${TARGET_NAME}.elf PRIVATE
        -mcpu=cortex-m0plus
        -mthumb
        --specs=nano.specs          # 精简 C 库(系统调用桩见 src/syscalls.c)
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
        -T${LD_SCRIPT}
    )

    # 后处理:烧录文件(hex/bin)输出到根目录 dist/
    set(ELF_FILE $<TARGET_FILE:${TARGET_NAME}.elf>)
    set(HEX_FILE ${DIST_DIR}/${TARGET_NAME}.hex)
    set(BIN_FILE ${DIST_DIR}/${TARGET_NAME}.bin)

    add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
        COMMAND ${CMAKE_OBJCOPY} -O ihex   ${ELF_FILE} ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILE} ${BIN_FILE}
        COMMAND ${CMAKE_SIZE} ${ELF_FILE}
        COMMENT "输出 ${TARGET_NAME}.hex / .bin 到 dist/,并打印内存占用"
    )
endfunction()

# ---- Bootloader:32KB @0x0 ----
hc32f072_add_firmware(hc32f072ka_boot
    ${SOURCE_DIR}/startup/hc32f072ka_boot.ld
    ${SOURCE_DIR}/bootloader/main.c
    ${SOURCE_DIR}/src/syscalls.c
)

# ---- App:96KB @0x8000(由 Bootloader 升级) ----
hc32f072_add_firmware(hc32f072ka_app
    ${SOURCE_DIR}/startup/hc32f072ka_app.ld
    ${SOURCE_DIR}/src/main.c
    ${SOURCE_DIR}/src/syscalls.c
)

# 便捷别名:cmake --build build --target firmware(构建全部镜像)
add_custom_target(firmware DEPENDS hc32f072ka_boot.elf hc32f072ka_app.elf)
