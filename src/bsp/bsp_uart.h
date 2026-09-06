/**
 * \file bsp_uart.h
 * \brief UART0 板级驱动接口(查询方式)
 *
 * 默认参数见 board.h:UART0 引脚(AF1)、波特率,8 数据位、无校验、1 停止位。
 */
#ifndef BSP_BSP_UART_H
#define BSP_BSP_UART_H

#include <stdint.h>

/**
 * \brief 初始化 UART0(8N1,查询收发)
 *
 * 打开 GPIO/UART0 时钟门控、配置 TX/RX 引脚复用并完成 Uart_Init()。
 *
 * \param  None
 * \return None
 */
void bsp_uart0_init(void);

/**
 * \brief 查询 UART0 是否已完成初始化
 *
 * 供 newlib 系统调用桩(_write 等)判断能否安全发送,避免初始化前误写。
 *
 * \param  None
 * \return uint8_t 1:已初始化;0:未初始化
 */
uint8_t bsp_uart0_is_ready(void);

/**
 * \brief 查询方式发送单字节
 *
 * \param  ch 待发送字节
 * \return None
 */
void bsp_uart0_put_char(uint8_t ch);

/**
 * \brief 发送字符串(遇到 '\0' 结束)
 *
 * \param  str 待发送的以 '\0' 结尾字符串
 * \return None
 */
void bsp_uart0_write(const char *str);

/**
 * \brief 格式化输出(等同 printf,写入 UART0)
 *
 * \param  fmt 格式字符串
 * \param  ... 变参
 * \return int 实际输出字符数(不含 '\0');出错为负值
 */
int bsp_uart0_printf(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

#endif /* BSP_BSP_UART_H */
