# HC32F072KA 固件工程(Bootloader 分支:串口 IAP 升级)

基于 **小华半导体(原华大 HDSC)HC32F072KA**(LQFP64)的嵌入式固件工程。
芯片板载**未接外部晶振**,系统时钟全部取自**内部 RC**,经 PLL 倍频到 48MHz。

本分支包含双镜像:**Bootloader(32KB)+ App(96KB)**,App 支持**串口在线升级(IAP)**。

## 特性

- 目标:HC32F072KA(LQFP64,128KB Flash / 16KB SRAM,Cortex-M0+)
- 系统时钟:内部 RCH 4MHz × 12(PLL)= **48MHz**,无外部晶振
- 双镜像:
  - `hc32f072ka_boot` **Bootloader @0x00000000(32KB)**:
    上电自检——App 请求魔数则停留引导;App 向量表合法则跳转;否则停留
  - `hc32f072ka_app` **App @0x00008000(96KB)**:
    LED 周期闪烁 + UART0 打印,串口输入 `boot` + 回车进入引导
- 串口 IAP:自定义二进制协议(帧 CRC16 + 整镜像 CRC32 校验),扇区级
  擦写、读回校验、断点可重试,协议见 `docs/iap_protocol.md`(仓库不含上位机
  程序,主机侧按协议自行实现)
- 构建:CMake + arm-none-eabi-gcc,产物 dist(hex/bin)与 build(elf/map)
- 风格:Linux 内核编程风格、Doxygen 注释、UTF-8 + LF

## 快速开始

前置:CMake ≥ 3.20、arm-none-eabi-gcc。

```sh
# Linux / macOS
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Windows PowerShell / 一键脚本
build.bat            # 或:build.bat clean / release / minsize
```

构建产物:

| 目录 | 文件 | 说明 |
| --- | --- | --- |
| `dist/` | `hc32f072ka_boot.hex/.bin` | Bootloader 烧录文件 |
| `dist/` | `hc32f072ka_app.hex/.bin` | App 烧录文件(也可用 IAP 升级) |
| `build/` | `hc32f072ka_{boot,app}.elf/.map` | 调试与内存占用检查 |

## 首次烧录(SWD)

1. 烧 Bootloader:`dist/hc32f072ka_boot.hex`(地址 0x0);
2. 烧初始 App:`dist/hc32f072ka_app.hex`(地址 0x8000);
3. 复位后串口(115200-8-N-1)应看到 App 每 500ms 打印 `tick N ...`。

## 串口升级(IAP)

1. App 运行中在串口输入 `boot` + 回车(或 App 区非法/为空时复位,自动停留
   引导模式);
2. Bootloader 进入引导后等待主机 IAP 帧(SYNC/WRITE/DONE),串口
   115200-8-N-1;
3. 主机侧按 `docs/iap_protocol.md` 的帧格式完成升级(仓库不内置上位机
   程序);Bootloader 校验通过后自动跳转新 App。

协议细节、失败恢复见 `docs/iap_protocol.md`。

## 板级配置(重要)

LED 与串口引脚集中在 `src/bsp/board.h`(默认 UART0 = PA09/PA10 AF1,
LED = PC13 占位),按实际原理图修改即可。Bootloader 与 App 共用同一份
板级配置;若两者使用不同引脚/波特率,请分别调整(需同步改主机侧参数)。

## 目录结构

```
├── AGENTS.md                开发纪律(Agent 与协作者必读)
├── CHANGELOG.md             修改记录
├── README.md                本文件
├── CMakeLists.txt           顶层构建(双镜像目标)
├── build.bat                Windows 一键构建脚本
├── cmake/                   GCC 交叉工具链文件
├── config/ddl_device.h      DDL 系列/封装定义
├── startup/                 GCC 启动文件与链接脚本(boot/app 两份)
├── src/                     App 源码(main、bsp、iap_shared.h、syscalls)
│   └── bsp/                 board.h、bsp_sysclk/bsp_uart/bsp_led
├── bootloader/              Bootloader(main + iap_proto 协议模块)
├── docs/iap_protocol.md     串口 IAP 协议与升级说明
└── third_party/             第三方(只读)
    ├── cmsis/               ARM CMSIS 5.9.0
    └── hc32f072_ddl/        官方 DDL Rev1.1.1 裁剪子集
```

## 时钟方案说明(无外部晶振)

内部 RCH(4/8/16/22.12/24MHz)+ PLL(8~48MHz)。本工程:

1. 上电默认 RCH 4MHz 运行;
2. `bsp_sysclk_init()`:切 RCL → 重配 RCH 4MHz → PLL(RCH ×12 = 48MHz)
   → Flash 等待 1 周期 → 切 PLL;
3. `SystemCoreClock` 刷新为 48MHz。

Bootloader 与 App 使用完全相同的时钟初始化代码。

## 烧录与调试

SWD(PA13/PA14 + nRST)烧录/调试。注意:
- Bootloader 与 App 两个镜像地址不同,务必选择对应 hex;
- 升级/调试过程中 App 若运行异常,可通过 SWD 重新烧录任意镜像恢复。

## 许可与第三方

工程自身代码无附加许可约束;`third_party/` 内容来源与许可详见
`THIRD_PARTY_NOTICES.md`。
