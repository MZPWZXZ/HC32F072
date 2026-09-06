# 修改记录(CHANGELOG)

> 本文件按时间顺序登记每一次修改(需求:每次修改均要做记录)。
> 格式:`### yyyy-mm-dd [序号] 简述` + 改动清单。
> 提交前必须保证 `cmake --build build` 验证通过。

---

### 2026-09-06 [001] 初始化 HC32F072KA 固件工程(首版)

- 内容:空仓库内搭建完整可编译的 HC32F072KA 固件 demo。
- 系统时钟:无外部晶振,内部 RCH 4MHz × 12(PLL)= 48MHz,
  HCLK = PCLK = 48MHz,Flash 等待 1 周期。
- 功能:LED 周期闪烁 + UART0(115200-8-N-1)轮询打印。
- 新增/改动文件:
  - 文档/规范:`AGENTS.md`、`README.md`、`CHANGELOG.md`、
    `THIRD_PARTY_NOTICES.md`、`.gitattributes`(UTF-8+LF)、`.gitignore`;
  - 构建:`CMakeLists.txt`、`cmake/arm-none-eabi-gcc.cmake`;
  - 配置:`config/ddl_device.h`(DDL 系列 HC32F072、封装 K=64PIN);
  - 启动/链接:`startup/startup_hc32f072.S`、`startup/hc32f072ka_flash.ld`
    (Flash 128KB@0x0、RAM 16KB@0x20000000);
  - 应用层:`src/main.c`、`src/syscalls.c`(newlib 最小系统调用桩,
    `_write` 在 UART0 就绪后重定向到串口)、`src/bsp/board.h`、
    `src/bsp/bsp_sysclk.{c,h}`、`src/bsp/bsp_led.{c,h}`、
    `src/bsp/bsp_uart.{c,h}`;
  - 第三方:`third_party/cmsis/`(CMSIS 5.9.0 core 头文件)、
    `third_party/hc32f072_ddl/`(官方 DDL Rev1.1.1 裁剪子集)。
- 说明:DDL 的 `system_hc32f072.h` 顶部“用户设置区”封装宏已按
  LQFP64 置为 `HC32F072Kxxx`(DDL 允许的用户配置项)。
- 验证:arm-none-eabi-gcc 15.3 + CMake 4.4 交叉编译通过,
  应用代码 `-Wall -Wextra -Werror` 零警告;查看 map 确认内存无溢出。

### 2026-09-06 [002] 新增 Windows 一键构建脚本 build.bat

- 需求:希望直接运行 .bat 即可完成编译,不用手敲 cmake 命令。
- 新增文件:
  - `build.bat`:自动探测 cmake / mingw32-make / arm-none-eabi-gcc
    (优先环境变量 `HC32F072_CMAKE`/`HC32F072_MAKE`,其次 PATH,
    最后本机 C:\Program Files\UserApp 目录),然后配置并构建;
  - 参数:`build.bat` = Debug,`build.bat release` = Release(-O2),
    `build.bat minsize` = MinSizeRel(-Os),`build.bat clean` = 删除 build 目录;
  - 注意:脚本刻意只用 ASCII 字符,保证任意系统代码页下可运行;
    采用仓库统一的 LF 行尾(经实测 cmd.exe 可正常执行)。
- 文档同步:AGENTS.md、README.md 的目录结构加入 build.bat,并在
  “构建与验证/快速开始”补充一键用法。
- 验证:在 PowerShell 中执行 `cmd /c build.bat clean`(清理成功)与
  `cmd /c build.bat`(配置+全量编译 exit=0,产物 elf/hex/bin/map 生成,
  size: text 12532 / data 84 / bss 336),均通过。
