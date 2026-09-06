/**
 * \file main.c
 * \brief HC32F072KA demo 主程序
 *
 * 功能:
 *   1. 系统时钟切换到 48MHz(内部 RCH → PLL,无外部晶振);
 *   2. 初始化板载 LED 与 UART0(115200-8-N-1);
 *   3. 上电打印一次启动信息;
 *   4. 主循环:LED 周期翻转,并按节拍打印计数。
 *
 * 引脚等板级参数集中在 src/bsp/board.h 中配置。
 */
#include "bsp_led.h"
#include "bsp_sysclk.h"
#include "bsp_uart.h"

#include "board.h"            /* BSP_UART0_BAUDRATE 等板级宏 */
#include "ddl.h"              /* delay1ms() */
#include "system_hc32f072.h"  /* SystemCoreClock */

/* LED 翻转周期(毫秒),主循环每周期翻转一次并打印一行 */
#define APP_BLINK_PERIOD_MS   (500u)

/**
 * \brief 上电启动信息打印
 *
 * 输出芯片/时钟/串口参数,便于确认内部时钟链路与串口是否工作。
 *
 * \param  None
 * \return None
 */
static void app_print_banner(void)
{
	bsp_uart0_write("\r\n");
	bsp_uart0_write("========================================\r\n");
	bsp_uart0_write(" HC32F072KA demo\r\n");
	bsp_uart0_write("----------------------------------------\r\n");
	/* newlib 下 uint32_t 为 unsigned long,显式转 unsigned int 打印 */
	bsp_uart0_printf(" SystemCoreClock : %u Hz\r\n",
			 (unsigned int)SystemCoreClock);
	bsp_uart0_printf(" PCLK            : %u Hz\r\n",
			 (unsigned int)Sysctrl_GetPClkFreq());
	bsp_uart0_printf(" Clock source    : internal RCH x12 PLL\r\n");
	bsp_uart0_printf(" UART0           : %u bps 8N1 poll mode\r\n",
			 (unsigned int)BSP_UART0_BAUDRATE);
	bsp_uart0_write("========================================\r\n");
}

/**
 * \brief 主函数(入口在 startup_hc32f072.S 中经 SystemInit 后调用)
 *
 * \param  None
 * \return int 正常不会返回;遵循 GCC 对 main 返回类型的约定
 */
int main(void)
{
	uint32_t tick_count = 0u;

	bsp_sysclk_init();   /* 内部 RCH × 12 PLL → 48MHz */
	bsp_led_init();      /* 板载 LED */
	bsp_uart0_init();    /* 调试串口 */

	app_print_banner();

	for (;;) {
		bsp_led_toggle();
		delay1ms(APP_BLINK_PERIOD_MS);
		tick_count++;
		bsp_uart0_printf("tick %u: LED toggled (sysclk %u Hz)\r\n",
				 (unsigned int)tick_count,
				 (unsigned int)SystemCoreClock);
	}
}
