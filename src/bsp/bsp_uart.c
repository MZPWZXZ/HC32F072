/**
 * \file bsp_uart.c
 * \brief UART0 板级驱动实现(查询方式)
 *
 * 使用 DDL 的 uart 驱动做 8N1 轮询收发;格式化输出借助
 * newlib-nano 的 vsnprintf() 先写入内部缓冲再逐字节轮询发送,
 * 不依赖任何系统调用/重定向,可在裸机下直接使用。
 */
#include "bsp_uart.h"

#include <stdarg.h>
#include <stdio.h>

#include "board.h"
#include "ddl.h"          /* Sysctrl_* / 基础类型 */
#include "gpio.h"         /* Gpio_* 引脚配置 */
#include "uart.h"         /* Uart_* 串口驱动 */
#include "sysctrl.h"      /* Sysctrl_* 时钟门控 */

#define BSP_UART0_PRINTF_BUF_LEN   (128u)   /* 格式化缓冲字节数 */

static uint8_t s_uart0_ready;   /* UART0 初始化完成标志(供 _write 使用) */

/**
 * \brief 配置并初始化一个 UART 引脚为复用功能
 *
 * \param  port    端口
 * \param  pin     引脚
 * \param  af      复用功能编号
 * \param  dir     方向(输入/输出)
 * \return None
 */
static void bsp_uart0_pin_cfg(en_gpio_port_t port, en_gpio_pin_t pin,
			      en_gpio_af_t af, en_gpio_dir_t dir)
{
	stc_gpio_cfg_t stc_gpio_cfg;

	DDL_ZERO_STRUCT(stc_gpio_cfg);
	stc_gpio_cfg.enDir = dir;
	Gpio_Init(port, pin, &stc_gpio_cfg);
	Gpio_SetAfMode(port, pin, af);
}

void bsp_uart0_init(void)
{
	stc_uart_cfg_t stc_uart_cfg;

	/* 外设时钟门控:GPIO 与 UART0 */
	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
	Sysctrl_SetPeripheralGate(SysctrlPeripheralUart0, TRUE);

	/* TX 引脚(输出)与 RX 引脚(输入)配为 UART0 复用功能 */
	bsp_uart0_pin_cfg(BSP_UART0_TX_PORT, BSP_UART0_TX_PIN,
			  BSP_UART0_TX_AF, GpioDirOut);
	bsp_uart0_pin_cfg(BSP_UART0_RX_PORT, BSP_UART0_RX_PIN,
			  BSP_UART0_RX_AF, GpioDirIn);

	/* 8 数据位、无校验、1 停止位(Mode1 即 8 位可变波特率模式) */
	DDL_ZERO_STRUCT(stc_uart_cfg);
	stc_uart_cfg.enRunMode      = UartMskMode1;
	stc_uart_cfg.enMmdorCk      = UartMskDataOrAddr;  /* 0:无奇偶校验 */
	stc_uart_cfg.enStopBit      = UartMsk1bit;
	stc_uart_cfg.stcBaud.enClkDiv = UartMsk16Or32Div; /* 16 倍过采样  */
	stc_uart_cfg.stcBaud.u32Pclk  = Sysctrl_GetPClkFreq();
	stc_uart_cfg.stcBaud.u32Baud  = BSP_UART0_BAUDRATE;
	Uart_Init(M0P_UART0, &stc_uart_cfg);

	/* 清残留状态标志 */
	Uart_ClrStatus(M0P_UART0, UartRC);
	Uart_ClrStatus(M0P_UART0, UartTC);

	s_uart0_ready = 1u;   /* 允许标准 printf()/_write() 向串口输出 */
}

uint8_t bsp_uart0_is_ready(void)
{
	return s_uart0_ready;
}

int bsp_uart0_get_char(uint8_t *ch)
{
	if (Uart_GetStatus(M0P_UART0, UartRC) != TRUE) {
		return 0;   /* 暂无接收数据 */
	}

	/* 先清接收标志再取数(与官方例程一致),防止重复触发 */
	Uart_ClrStatus(M0P_UART0, UartRC);
	*ch = Uart_ReceiveData(M0P_UART0);
	return 1;
}

void bsp_uart0_put_char(uint8_t ch)
{
	Uart_SendDataPoll(M0P_UART0, ch);
}

void bsp_uart0_write(const char *str)
{
	while (*str != '\0')
		bsp_uart0_put_char((uint8_t)(*str++));
}

int bsp_uart0_printf(const char *fmt, ...)
{
	char buf[BSP_UART0_PRINTF_BUF_LEN];
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (len > 0)
		bsp_uart0_write(buf);

	return len;
}
