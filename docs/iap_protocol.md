# 串口 IAP 协议说明

HC32F072KA Bootloader 分支:上电先运行 Bootloader(32KB @0x00000000),
App 位于 0x00008000(96KB)。App 运行中输入 `boot` + 回车即软复位进入
Bootloader,等待主机通过串口按其帧格式完成固件升级。

> 说明:本仓库只包含设备端(Bootloader/App)代码,不内置上位机程序。
> 需要升级时,主机侧按本文档第 3 节的帧格式自行实现(或用支持
> 二进制收发与 CRC 的串口工具/脚本)。

## 1. Flash 分区与镜像

| 镜像 | 链接基址 | 大小 | 产物(dist/) |
| --- | --- | --- | --- |
| Bootloader | 0x00000000 | 32KB | `hc32f072ka_boot.hex/.bin` |
| App | 0x00008000 | 96KB | `hc32f072ka_app.hex/.bin` |

RAM 前 64B(0x20000000~0x2000003F)保留:0x20000000 存放 IAP 魔数
`0x49504121("IAP!")`,App 置魔数后 `NVIC_SystemReset()`,Bootloader 检测到
魔数即停留在引导模式(两个镜像的链接脚本均已把运行区抬到 0x20000040)。

依据 HC32F072 的 Flash 特性(官方应用笔记):
- 扇区(页)512B,擦除粒度 512B;
- 容量 >32KB 的器件要求**擦写函数运行在 0~32K 区域**——Bootloader 镜像
  整体位于 0x0~0x7FFF,天然满足;
- 在 Flash 内执行擦除/编程时,硬件自动暂停 CPU 等待 BUSY,无需搬 RAM;
- Flash 寄存器写保护:BYPASS(0x5A5A/0xA5A5)序列由 DDL 驱动内部处理。

## 2. 串口参数

115200 bps,8 数据位、无校验、1 停止位(8N1);引脚默认 PA09=TX、PA10=RX
(见 `src/bsp/board.h`)。Bootloader 与 App 均使用内部时钟 48MHz。

## 3. 帧格式(自定义二进制协议)

```
字节序:小端
[0]      MAGIC    0xAA
[1]      CMD      命令
[2..3]   LEN      负载长度 u16(不含帧头与 CRC)
[4..]    PAYLOAD  负载
[末2]    CRC16    CRC16-CCITT-FALSE,覆盖 CMD|LEN|PAYLOAD
```

回复:单字节状态——`0x06` ACK 成功 / `0x15` NAK 失败。主机**等收到 ACK
才发下一帧**。

| CMD | 值 | 负载 | 说明 |
| --- | --- | --- | --- |
| SYNC | 0x01 | 空 | 握手,引导就绪后回 ACK |
| WRITE | 0x03 | offset:u32 + 512B 扇区数据 | 擦除该扇区→编程→读回校验,ACK/NAK |
| DONE | 0x04 | size:u32 + crc32:u32 | 回读整镜像校验 CRC32(IEEE/zlib 标准),通过则跳转 App |
| REBOOT | 0x05 | 空 | 不校验直接跳转 App |

整帧最长 522 字节(WRITE 帧)。

## 4. 固件侧升级行为(供主机参考)

1. 先保证 Bootloader 已烧入(0x0,SWD 烧 `dist/hc32f072ka_boot.hex`);
2. App 运行中输入 `boot` + 回车(或 App 区非法时复位自动停留引导);
3. 主机流程建议:SYNC 握手 → 按**从尾到头**逐扇区 WRITE(向量表扇区
   最后写,中途断电/失败可保留旧 App)→ DONE(镜像长度 + 整镜像 CRC32)
   → Bootloader 回读校验 → 自动跳转新 App。

## 5. 失败与恢复(固件侧行为)

- 串口无响应:确认已进入 Bootloader(App 里输 `boot`;App 非法/为空时复位
  自动停留);检查串口与波特率是否 115200-8-N-1;
- WRITE 收到 NAK:该扇区“擦→写→校验”失败,主机可重发同一扇区
  (每扇区幂等,擦后重写);仍失败请检查供电/接线;
- DONE 校验失败:旧 App(向量扇区未被覆盖时)仍可运行,重新执行升级即可;
- 想强行走引导:App 内再输 `boot`,或复位后重新进入。

## 6. 关键源文件

| 文件 | 作用 |
| --- | --- |
| `src/iap_shared.h` | 分区/魔数/扇区等共享常量(两端唯一事实来源) |
| `bootloader/iap_proto.{h,c}` | Bootloader 侧协议/Flash 驱动封装 |
| `src/main.c` | App 控制台 `boot` 命令 |
| `startup/hc32f072ka_{boot,app}.ld` | 两镜像链接脚本 |
