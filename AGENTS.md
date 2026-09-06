# AGENTS.md — 仓库工作准则

本文件面向后续参与本仓库开发的人类协作者与 AI 智能体(Agent),
是**必须遵守**的工作纪律。开发前请先完整阅读本文档与 `CHANGELOG.md`。

## 1. 项目简介

- 目标芯片:**HC32F072KA**(小华/华大 HDSC,HC32F072 系列,LQFP64 封装)。
  资源:ARM Cortex-M0+ @48MHz、128KB Flash、16KB SRAM。
- **板上未接外部晶振**:所有时钟源只能使用芯片内部 RC。
- 系统主频方案:RCH(内部高速 RC)**4MHz × 12(PLL)= 48MHz**,
  HCLK = PCLK = 48MHz(1 分频),Flash 读等待 1 周期(>24MHz 必须 ≥1)。
- **当前为 Bootloader 分支,双镜像结构**:
  - `hc32f072ka_boot` Bootloader @0x00000000(32KB),串口 IAP 升级;
  - `hc32f072ka_app` App @0x00008000(96KB),LED 闪烁 + UART0 打印,
    串口输入 `boot`+回车进入引导;
  - 串口 IAP 升级:仓库不含上位机程序,主机侧按 `docs/iap_protocol.md`
    的帧格式自行实现。
- 工具链:GCC(arm-none-eabi-gcc)+ CMake,不使用任何 IDE 工程文件。

## 2. 目录结构

```
├── AGENTS.md                本文件(开发纪律)
├── README.md                工程说明/快速开始
├── CHANGELOG.md             修改记录(每次修改必须登记)
├── THIRD_PARTY_NOTICES.md   第三方代码来源/许可登记
├── .gitattributes           统一 UTF-8 + LF
├── CMakeLists.txt           顶层构建脚本(双镜像)
├── build.bat                Windows 一键构建脚本
├── dist/                    烧录文件输出目录(boot/app 的 hex/bin,不入库)
├── cmake/                   CMake 交叉编译工具链文件
├── config/                  ddl_device.h(DDL 系列/封装配置)
├── startup/                 GCC 启动文件 + boot/app 两份链接脚本
├── src/                     App 源码(main.c、iap_shared.h、syscalls.c、bsp/)
│   └── bsp/                 board.h 引脚配置与板级驱动
├── bootloader/              Bootloader(main.c、iap_proto.{c,h})
├── docs/iap_protocol.md     串口 IAP 协议与升级说明
└── third_party/             第三方代码(只读)
    ├── cmsis/               ARM CMSIS 5.9.0 core 头文件
    └── hc32f072_ddl/        官方 DDL Rev1.1.1 裁剪拷贝
```

## 3. 构建与验证

前置要求:CMake ≥ 3.20、arm-none-eabi-gcc(Linux/macOS 需 make;
Windows 下需 mingw32-make 或 ninja)。

```sh
# 配置(Linux)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
      -DCMAKE_BUILD_TYPE=Debug

# 配置(Windows PowerShell)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake `
      -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles"

# 构建
cmake --build build
```

> Windows 上更简单:直接运行根目录的 `build.bat`
> (等价于上面的 cmake 命令,自动探测 cmake/mingw32-make/arm 工具链;
> 参数:`clean` 清理、`release`/`minsize` 选择构建类型)。

产物分两处存放(`dist/` 与 `build/` 同级,位于仓库根目录):
`dist/` 下为烧录用 `hc32f072ka_boot.hex/.bin` 与
`hc32f072ka_app.hex/.bin`(Bootloader 与 App 各一组);
`build/` 下为调试用对应 `.elf` 与 `.map`。

**“测试没有问题”的定义**(本仓库开发期无法上电运行,以编译级验证为准):

1. 编译、链接零错误(boot 与 app 两个目标);
2. 应用代码 `src/`、`bootloader/` 零警告(已开 `-Wall -Wextra -Werror`);
3. `*.map` 中 Flash/RAM 占用合理且无溢出(注意 RAM 运行区起点为
   0x20000040,头部 0x40B 保留);
4. 若改动了链接脚本/启动文件,检查 `.hex` 起始地址与向量表
   (boot @0x0,app @0x8000)。

## 4. 硬性编码规范

1. **编码与行尾**:所有文本文件 UTF-8(无 BOM)、行尾 LF,
   由 `.gitattributes` 强制,不要写入 CRLF/GBK 内容。
2. **Linux 编程风格**:
   - Tab 缩进(宽度 8 对齐),行宽尽量 ≤ 80 列;
   - 函数定义左大括号独占一行;`if/for/while` 等控制语句左大括号同行;
   - 标识符一律 snake_case(第三方 DDL 的驼峰命名除外);
   - 宏名全大写,常量优先 `#define` 或 `enum`;
   - 允许且鼓励使用 `static` 局部函数,不写全局变量(必须时用 `extern` 显式声明)。
3. **Doxygen 注释**:
   - 每个 `.c/.h` 文件头部:`/** \file xxx.c ... \brief 简介 ... */`;
   - 每个函数之前:`/** \brief 功能 \param 参数 \return 返回值 */`;
   - **无参数写 `\param None`,无返回值写 `\return None`**;
   - 关键宏/结构体也要有注释;注释要说明意图(为什么),不写废话。
4. **修改纪律(强制)**:
   - 每次修改前:`git status` + 阅读 `CHANGELOG.md` 最新条目;
   - 每次代码/配置/文档改动,先向 `CHANGELOG.md` 追加一条记录
     (格式:`### yyyy-mm-dd 序号 简述`,内容含改动文件与原因);
   - 修改完成后必须按第 3 节重新构建验证**通过**后才能提交;
   - 提交信息遵循 Conventional Commits 风格
     (`feat:`, `fix:`, `docs:`, `build:`, `chore:`, `refactor:`,
     `test:`),正文简述改动与验证结果;
   - 一次提交只包含一个逻辑变更。

## 5. third_party 纪律(只读)

- `third_party/hc32f072_ddl/`:华大小华官方 DDL Rev1.1.1 的裁剪拷贝
  (来源与裁剪见 `THIRD_PARTY_NOTICES.md`)。**除以下一项外禁止改动**:
  `mcu/common/system_hc32f072.h` 顶部“用户设置区”的封装宏,本项目已按
  LQFP64(64PIN)置为 `#define HC32F072Kxxx`。
- `third_party/cmsis/`:ARM CMSIS 5.9.0 官方头文件,原样拷贝,禁止改动。
- 新增任何第三方代码必须先登记到 `THIRD_PARTY_NOTICES.md`。

## 6. 常见坑(开发备忘)

- HCLK 大于 24MHz 时,**必须先** `Flash_WaitCycle(FlashWaitCycle1)`,
  再切换时钟源,否则程序随机跑飞;
- 修改 RCH 频率或使能 PLL 前,先把系统时钟切到内部低速 RCL
  (参见 `src/bsp/bsp_sysclk.c` 内注释的官方推荐流程);
- RCH 属于 RC 振荡器,频率精度有限,串口长帧/高波特率需考虑误差;
- **IAP/Flash**:
  - 分区常量唯一来源是 `src/iap_shared.h`,改它必须同步改两份链接脚本;
  - 擦除/编程前先 `Flash_Init(12, TRUE)`(48MHz 时间参数);
  - HC32F072(>32KB)要求 Flash 擦写代码位于 0~32K——只能放在 Bootloader;
  - 升级写盘“从尾到头”,向量表扇区最后写,中途失败可保留旧 App;
  - RAM 0x20000000~0x3F 为 IAP 保留区,普通代码不得使用;
  - 主机侧协议实现与 `bootloader/iap_proto.h` 的常量必须保持同步;
- 注释/日志中不得出现非 UTF-8 字符;字符串字面量含中文时确保源文件为 UTF-8。
