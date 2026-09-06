/**
 * \file iap_proto.h
 * \brief 串口 IAP 自定义协议模块(帧格式/命令定义)
 *
 * 协议与上位机 tools/iap_upload.py 严格对应,修改时两端必须同步。
 * 帧格式(共 max 522 字节):
 *   [0]      MAGIC   = 0xAA
 *   [1]      CMD
 *   [2..3]   LEN     = payload 字节数(u16 LE)
 *   [4..]    PAYLOAD
 *   [末2]    CRC16-CCITT-FALSE,覆盖 CMD|LEN|PAYLOAD
 *
 * 回复:单字节状态 ACK=0x06 / NAK=0x15,主机收到 ACK 才发下一帧。
 *
 * 写盘策略:主机按“从尾到头”逐扇区(512B)下发,每帧 WRITE 内部完成
 * “擦除本扇区→写→读回校验”,扇区级幂等可安全重试;首扇区(向量表)
 * 最后写入,中途失败可保留旧 App 继续运行。
 */
#ifndef BOOTLOADER_IAP_PROTO_H
#define BOOTLOADER_IAP_PROTO_H

#include <stdint.h>

#include "iap_shared.h"   /* IAP_FLASH_SECTOR_SIZE 等分区常量 */

/* 帧格式常量 */
#define IAP_FRAME_MAGIC        (0xAAu)   /* 帧头               */
#define IAP_CMD_SYNC           (0x01u)   /* 握手(空 payload)   */
#define IAP_CMD_WRITE          (0x03u)   /* 写一个扇区         */
#define IAP_CMD_DONE           (0x04u)   /* 结束并校验+跳转    */
#define IAP_CMD_REBOOT         (0x05u)   /* 重启到 App         */
#define IAP_REPLY_ACK          (0x06u)   /* 成功               */
#define IAP_REPLY_NAK          (0x15u)   /* 失败               */

#define IAP_HEADER_LEN         (4u)      /* MAGIC+CMD+LEN      */
#define IAP_CRC_LEN            (2u)      /* CRC16 长度         */
/* WRITE payload = offset(4) + 扇区数据(512),整帧 = 4+516+2 = 522 */
#define IAP_WRITE_PAYLOAD_LEN  (4u + IAP_FLASH_SECTOR_SIZE)
#define IAP_FRAME_MAX_LEN      (IAP_HEADER_LEN + IAP_WRITE_PAYLOAD_LEN + \
                                IAP_CRC_LEN)

/**
 * \brief IAP 模块初始化(按 48MHz HCLK 配置 Flash 编程时间参数)
 *
 * 必须在首次擦写 Flash 前调用一次。
 *
 * \param  None
 * \return None
 */
void iap_init(void);

/**
 * \brief 判断 App 区向量表是否合法(被擦除区 0xFF 自然判为非法)
 *
 * \param  None
 * \return uint8_t 1 合法;0 非法
 */
uint8_t iap_app_is_valid(void);

/**
 * \brief 跳转到 App(重置 MSP 后进入 App 复位向量)
 *
 * \param  None
 * \return None
 */
void iap_jump_to_app(void);

/**
 * \brief 轮询接收并处理一帧(自带超时与丢帧重同步)
 *
 * \param  timeout_ms 等待帧头的超时毫秒数
 * \return int 1 处理完一帧;0 超时无帧
 */
int iap_recv_process(uint32_t timeout_ms);

#endif /* BOOTLOADER_IAP_PROTO_H */
