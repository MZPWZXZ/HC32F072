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

### 2026-09-06 [004] Bootloader 分支:Flash 分区 + 双镜像工程骨架

- 需求:新建 Bootloader 分支,串口升级 App。
- 决策(已确认):Bootloader 32KB@0x0,App @0x8000(96KB);
  自定义串口协议 + Python 上位机;App 内串口命令进入引导。
- 改动:
  - `src/iap_shared.h`:新增 Boot/App 共享常量(分区/魔数/扇区粒度),要求
    与 startup 下链接脚本一一对应;
  - `startup/hc32f072ka_app.ld`:App 镜像 @0x8000 96KB;
  - `startup/hc32f072ka_boot.ld`:Boot 镜像 @0x0 32KB;两者 RAM 运行区统一
    从 0x20000040 开始(头部 0x40B 保留给 IAP 引导标志);
  - `CMakeLists.txt`:重构为双目标(hc32f072ka_boot / hc32f072ka_app),
    公共 BSP 收进 hc32f072_core;syscalls.c 直接编入各可执行文件
    (避免静态库链接顺序导致 _sbrk 无法解析);dist 输出两组 hex/bin;
  - `bootloader/main.c`:最小骨架——魔数判断停留在引导 / App 向量表合法则
    跳转 / 非法则停留并闪烁 LED;
  - 删除原单镜像链接脚本 `startup/hc32f072ka_flash.ld`(由 app/boot 两份替代);
  - 依据官方“FLASH 操作说明”:扇区 512B、擦写代码须位于 0~32K(Boot 区
    满足)、寄存器写保护需 BYPASS 序列、flash 内执行擦除硬件自动等待 BUSY。
- 验证:双目标编译零错误零警告;boot hex 基址 0x0、app hex 基址 0x8000;
  size boot text 12740 / app text 12532。

### 2026-09-06 [003] 编译产物 hex/bin 改输出到根目录 dist(与 build 同级)

- 需求:希望新建 `dist/` 文件夹专门存放编译后的 bin/hex,与 `build/` 同级。
- 改动:
  - `CMakeLists.txt`:新增 `DIST_DIR = ${CMAKE_SOURCE_DIR}/dist`;
    后处理命令先 `make_directory` 创建 dist(不存在时自动建),再把
    `.hex`/`.bin` 输出到 `dist/`;`.elf`/`.map` 仍留在 `build/`;
  - `build.bat`:新增 `DIST_DIR`;`clean` 参数同时删除 build 与 dist;
    结束回显区分 elf/map(build/)与 hex/bin(dist/);
  - `.gitignore`:忽略 `/dist/`(产物不入库);
  - 文档:README.md/AGENTS.md 的产物说明、目录结构、烧录路径同步更新。
- 验证:`cmd /c build.bat clean` 同时清除 build/dist 后,执行
  `cmd /c build.bat` 全量编译 exit=0,`dist/hc32f072ka.hex` 与
  `dist/hc32f072ka.bin` 生成于仓库根目录(与 build 同级),校验无误。
