# HC32F072KA 固件工程(无外部晶振,GCC + CMake)

基于 **小华半导体(原华大 HDSC)HC32F072KA**(LQFP64)的嵌入式固件 demo 工程。
芯片板载**未接外部晶振**,因此系统时钟全部取自**内部 RC**,经 PLL 倍频到满速 48MHz。

## 特性

- 目标:HC32F072KA(HC32F072 系列,LQFP64,128KB Flash / 16KB SRAM,Cortex-M0+)
- 系统时钟:内部 RCH 4MHz × 12(PLL)= **48MHz**,HCLK = PCLK = 48MHz,无外部晶振
- demo 功能:LED 周期闪烁 + UART0 轮询打印(默认 115200-8-N-1)
- 驱动:官方 DDL Rev1.1.1(裁剪子集)+ CMSIS 5.9.0,均置于 `third_party/`
- 构建:CMake + arm-none-eabi-gcc,无 IDE 依赖;产物 elf/hex/bin/map
- 风格:Linux 内核编程风格、Doxygen 注释、UTF-8 + LF(由 `.gitattributes` 固化)

## 快速开始

前置:CMake ≥ 3.20、arm-none-eabi-gcc。

```sh
# Linux / macOS
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Windows PowerShell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake `
      -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles"
cmake --build build
```

> **Windows 一键编译**:直接双击或运行根目录的 `build.bat`
> (自动探测 cmake / mingw32-make / arm-none-eabi-gcc,无需手敲命令)。
> 可用参数:`build.bat`(Debug)、`build.bat release`(-O2)、
> `build.bat minsize`(-Os)、`build.bat clean`(清理 build 与 dist 目录)。

构建产物分两处存放(`dist/` 与 `build/` 同级,位于仓库根目录):

| 目录 | 文件 | 说明 |
| --- | --- | --- |
| `dist/` | `hc32f072ka.hex` | Intel HEX(烧录用),编译后自动生成 |
| `dist/` | `hc32f072ka.bin` | 裸二进制(烧录用),编译后自动生成 |
| `build/` | `hc32f072ka.elf` | 可调试 ELF |
| `build/` | `hc32f072ka.map` | 链接 MAP(内存占用检查) |

## 板级配置(重要)

板上 LED 与 UART0 的引脚因板而异,统一在 `src/bsp/board.h` 集中配置,
按你的原理图修改即可:

```c
#define BSP_LED1_PORT          GpioPortC
#define BSP_LED1_PIN           GpioPin13   /* 默认占位引脚,请按实际板子修改 */
#define BSP_LED_ACTIVE_HIGH    1u          /* 1:高电平点亮;0:低电平点亮 */
#define BSP_UART0_TX_PORT      GpioPortA
#define BSP_UART0_TX_PIN       GpioPin9    /* UART0_TXD AF1 */
#define BSP_UART0_TX_AF        GpioAf1
#define BSP_UART0_RX_PORT      GpioPortA
#define BSP_UART0_RX_PIN       GpioPin10   /* UART0_RXD AF1 */
#define BSP_UART0_RX_AF        GpioAf1
#define BSP_UART0_BAUDRATE     115200u
```

> 默认 UART0 引脚 PA09/PA10(AF1)取自官方 DDL 的 64PIN(K 封装)例程,
> LED 默认占位 PC13。上板前请按实际硬件核对。

## 目录结构

```
├── AGENTS.md                开发纪律(Agent 与协作者必读)
├── CHANGELOG.md             修改记录
├── CMakeLists.txt           顶层构建
├── build.bat                Windows 一键构建脚本
├── dist/                    烧录文件输出目录(hex/bin,自动生成,不入库)
├── cmake/                   GCC 交叉工具链文件
├── config/ddl_device.h      DDL 系列/封装定义
├── startup/                 GCC 启动文件与链接脚本(128K Flash/16K RAM)
├── src/                     main.c、syscalls.c(newlib 桩)+ bsp/ 应用层
│   └── bsp/                 board.h、bsp_sysclk/bsp_uart/bsp_led
└── third_party/             第三方(只读)
    ├── cmsis/               ARM CMSIS 5.9.0
    └── hc32f072_ddl/        官方 DDL Rev1.1.1 裁剪子集
```

## 时钟方案说明(无外部晶振)

HC32F072 内部振荡器:RCH(内部高速 RC,可配 4/8/16/22.12/24MHz)、
RCL(内部低速 RC 32.8/38.4kHz)、PLL(输出 8~48MHz,输入须来自 RCH 或 XTH)。

由于不接外部晶振,本工程:

1. 上电默认 RCH 4MHz 运行;
2. `bsp_sysclk_init()` 按官方推荐流程:切到 RCL → 重配 RCH 4MHz →
   配置 PLL(输入 RCH 4MHz,×12 = 48MHz)→ Flash 等待 1 周期 → 切到 PLL 48MHz;
3. `SystemCoreClock` 随之刷新为 48MHz,`delay1ms()` 等延时按它工作。

## 烧录与调试

芯片支持 SWD(PA13/PA14 + nRST)。可使用 J-Link / DAP-Link 等烧录
`dist/hc32f072ka.hex`(注意保持芯片为 SWD 模式,勿把 SWD 引脚复用为 GPIO)。

## 许可与第三方

- 工程自身代码无附加许可约束,引用时请保留文件头注释。
- `third_party/` 内容来源与许可详见 `THIRD_PARTY_NOTICES.md`。
