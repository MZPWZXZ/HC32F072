# ----------------------------------------------------------------------------
# arm-none-eabi-gcc 交叉编译工具链文件
#
# 用法:
#   cmake -S . -B build \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
#         -DCMAKE_BUILD_TYPE=Debug
#
# 说明:
#   - CMAKE_SYSTEM_NAME 置为 Generic:纯裸机交叉编译,不做系统探测;
#   - CMAKE_TRY_COMPILE_TARGET_TYPE 置为 STATIC_LIBRARY:
#     交叉编译环境下跳过链接型 try_compile,避免找不到运行库;
#   - 若工具链不在 PATH 中,可传 -DTOOLCHAIN_PREFIX=/path/to/arm-none-eabi-
# ----------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT DEFINED TOOLCHAIN_PREFIX)
    set(TOOLCHAIN_PREFIX "arm-none-eabi-")
endif()

set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc")
# 汇编(.S)同样用 gcc 预处理后汇编
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 配套 binutils(交叉编译时 CMake 不会自动寻找,需显式给出)
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy" CACHE FILEPATH "GNU objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}objdump" CACHE FILEPATH "GNU objdump")
set(CMAKE_SIZE    "${TOOLCHAIN_PREFIX}size"    CACHE FILEPATH "GNU size")
set(CMAKE_NM      "${TOOLCHAIN_PREFIX}nm"      CACHE FILEPATH "GNU nm")
set(CMAKE_AR      "${TOOLCHAIN_PREFIX}ar"      CACHE FILEPATH "GNU ar")

# 交叉编译时无需(也不应)进行运行期特性测试
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)
