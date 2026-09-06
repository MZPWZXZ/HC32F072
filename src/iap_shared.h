/**
 * \file iap_shared.h
 * \brief Bootloader/App 共享的 IAP(串口升级)常量与分区定义
 *
 * Bootloader 与 App 两个镜像都必须以本文件为准,保证:
 *   - Flash 分区一致(本文件与 startup 目录下的链接脚本一一对应);
 *   - RAM 保留区一致(两个链接脚本都把 RAM 起点抬到 0x20000040,
 *     前 0x40 字节专用于跨软复位的“进入引导”标志);
 *   - 协议/魔数一致。
 */
#ifndef SRC_IAP_SHARED_H
#define SRC_IAP_SHARED_H

/* ============================ Flash 分区 ============================ */
#define IAP_BOOT_BASE         (0x00000000u)   /* Bootloader 起始(向量表)  */
#define IAP_BOOT_SIZE         (0x00008000u)   /* Bootloader 大小:32KB     */
#define IAP_APP_BASE          (0x00008000u)   /* App 起始                 */
#define IAP_APP_MAX_SIZE      (0x00018000u)   /* App 最大大小:96KB        */
#define IAP_FLASH_END         (0x0001FFFFu)   /* 片内 Flash 末尾(128KB)   */

/* ============================ RAM 保留区 ============================ */
#define IAP_RAM_BASE          (0x20000000u)   /* SRAM 实际基址            */
#define IAP_RAM_RESERVED      (0x00000040u)   /* 头部保留 64B(与链接脚本一致) */
#define IAP_MAGIC_ADDR        (0x20000000u)   /* 引导标志地址             */
#define IAP_MAGIC_WORD        (0x49504121u)   /* 引导魔数 "IAP!"          */

/* ============================ Flash 特性 ============================ */
#define IAP_FLASH_SECTOR_SIZE (512u)          /* 扇区(页)擦除粒度:512B   */

#endif /* SRC_IAP_SHARED_H */
