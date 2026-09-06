# 第三方代码登记(THIRD_PARTY_NOTICES)

本仓库 `third_party/` 下的代码不是本工程自研,均按来源原样(或最小裁剪)引入,
以下逐一登记来源与许可。引入新第三方代码前必须先在此登记。

## 1. hc32f072_ddl — 小华半导体(原华大 HDSC)HC32F072 官方驱动库

- 组件:HC32F072 Device Driver Library **Rev1.1.1**(2020-06-02 发布)。
- 来源:官方发布包 `hc32f072_ddl_Rev1.1.1.zip`
  (从 GitHub 镜像仓库 `Edragon/MCU-HDSC-SDK` 的 `HC32F072_SDK.rar` 中解出)。
- 裁剪方式:仅保留本工程编译所需文件:
  - `mcu/common/`(设备头、系统时钟、中断管理、基础类型、EVB 板级宏头);
  - `driver/inc/`(全部 31 个驱动头文件,便于后续扩展);
  - `driver/src/`(仅 `ddl.c sysctrl.c gpio.c uart.c flash.c` 五个源文件,
    其余外设驱动未拷贝,需要时再按官方包补充)。
- 许可:华大/小华版权声明与免责条款见各源文件头部(禁止用于非 HC 器件等),
  无附加署名要求;保留全部原始版权头,未改动。
- **允许的用户配置改动(唯一)**:`mcu/common/system_hc32f072.h` 顶部
  “START OF USER SETTINGS HERE”区域的封装宏,本项目已置为
  `#define HC32F072Kxxx`(64PIN,LQFP64)。
  其余任何 DDL 文件一律视为只读,不得修改。

## 2. cmsis — ARM CMSIS 5.9.0(仅 Core Include)

- 组件:CMSIS Core 头文件(`core_cm0plus.h`、`cmsis_compiler.h`、
  `cmsis_gcc.h`、`cmsis_armcc.h`、`cmsis_armclang.h`、`cmsis_iccarm.h`、
  `cmsis_version.h`)。
- 来源:GitHub `ARM-software/CMSIS_5` tag `5.9.0`,
  目录 `CMSIS/Core/Include/`(raw.githubusercontent.com 原样下载)。
- 许可:Apache License 2.0(文件头含版权与许可文本)。
- 说明:HC32F072 官方设备头 `HC32F072.h` 依赖 CMSIS core 定义
  (`__IO/__WEAK/SysTick/NVIC` 等),故引入;不包含 CMSIS-DSP/RTOS 部分。
