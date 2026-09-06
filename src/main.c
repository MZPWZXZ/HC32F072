/**
 * \file main.c
 * \brief HC32F072KA App 主程序(Bootloader 分支,运行于 0x8000)
 *
 * 功能:
 *   1. 系统时钟切换到 48MHz(内部 RCH → PLL,无外部晶振);
 *   2. 初始化板载 LED 与 UART0(115200-8-N-1);
 *   3. LED 每 500ms 翻转并打印 tick;
 *   4. 串口控制台:输入 boot(回车)置 IAP 魔数并软复位,进入 Bootloader。
 *
 * 引脚等板级参数集中在 src/bsp/board.h;分区常量见 src/iap_shared.h。
 */
#include "bsp_led.h"
#include "bsp_sysclk.h"
#include "bsp_uart.h"

#include "board.h"
#include "ddl.h"              /* delay1ms() */
#include "iap_shared.h"
#include "system_hc32f072.h"

#define APP_BLINK_PERIOD_MS   (500u)   /* LED 翻转/打印周期   */
#define APP_HEARTBEAT_MS      (10u)    /* 主循环心跳(控制台轮询粒度) */
#define APP_LINE_BUF_SIZE     (32u)    /* 串口命令行缓冲       */
#define APP_BOOT_CMD          "boot"   /* 进入引导的命令       */

/* 串口命令行缓冲 */
static char     s_line[APP_LINE_BUF_SIZE];
static uint8_t  s_line_len;

/**
 * \brief 字符串相等判断(避免引入 string 库依赖)
 *
 * \param  a 字符串 a
 * \param  b 字符串 b
 * \return uint8_t 1 相等;0 不等
 */
static uint8_t app_str_eq(const char *a, const char *b)
{
	while ((*a != '\0') && (*b != '\0')) {
		if (*a != *b) {
			return 0u;
		}
		a++;
		b++;
	}
	return (*a == '\0' && *b == '\0') ? 1u : 0u;
}

/**
 * \brief 请求进入 Bootloader:置 IAP 魔数后软复位
 *
 * RAM 在软复位后内容保留,Bootloader 检测到魔数即停留在引导模式。
 *
 * \param  None
 * \return None
 */
static void app_enter_bootloader(void)
{
	volatile uint32_t *p_magic = (volatile uint32_t *)IAP_MAGIC_ADDR;

	bsp_uart0_write("\r\n>> enter bootloader, reset ...\r\n");
	*p_magic = IAP_MAGIC_WORD;
	__DSB();
	NVIC_SystemReset();

	for (;;) {
		;   /* 正常不会到达 */
	}
}

/**
 * \brief 处理一行控制台命令
 *
 * \param  line 以 '\0' 结尾的命令行
 * \return None
 */
static void app_console_line(const char *line)
{
	if (app_str_eq(line, APP_BOOT_CMD) != 0u) {
		app_enter_bootloader();
		return;
	}

	bsp_uart0_write("\r\n");
	bsp_uart0_write("unknown cmd: ");
	bsp_uart0_write(line);
	bsp_uart0_write("\r\n");
	bsp_uart0_write("help: type 'boot' + Enter to enter bootloader\r\n");
}

/**
 * \brief 控制台字符输入(行缓冲,回车执行)
 *
 * \param  ch 收到的字节
 * \return None
 */
static void app_console_input(uint8_t ch)
{
	if ((ch == (uint8_t)'\r') || (ch == (uint8_t)'\n')) {
		/* 回车:执行当前行 */
		bsp_uart0_write("\r\n");
		s_line[s_line_len] = '\0';
		if (s_line_len > 0u) {
			app_console_line(s_line);
		}
		s_line_len = 0u;
		return;
	}

	if ((ch == 0x08u) || (ch == 0x7Fu)) {
		/* 退格 */
		if (s_line_len > 0u) {
			s_line_len--;
		}
		return;
	}

	if (s_line_len < (APP_LINE_BUF_SIZE - 1u)) {
		bsp_uart0_put_char(ch);   /* 回显 */
		s_line[s_line_len++] = (char)ch;
	}
}

/**
 * \brief 上电启动信息打印
 *
 * \param  None
 * \return None
 */
static void app_print_banner(void)
{
	bsp_uart0_write("\r\n");
	bsp_uart0_write("========================================\r\n");
	bsp_uart0_write(" HC32F072KA app v1.0 (IAP build)\r\n");
	bsp_uart0_write("----------------------------------------\r\n");
	bsp_uart0_printf(" App base       : 0x%X\r\n",
			 (unsigned int)IAP_APP_BASE);
	bsp_uart0_printf(" SystemCoreClock: %u Hz\r\n",
			 (unsigned int)SystemCoreClock);
	bsp_uart0_printf(" UART0          : %u bps 8N1\r\n",
			 (unsigned int)BSP_UART0_BAUDRATE);
	bsp_uart0_write(" type 'boot' + Enter to enter bootloader\r\n");
	bsp_uart0_write("========================================\r\n");
}

/**
 * \brief 主函数(入口在 startup_hc32f072.S 中经 SystemInit 后调用)
 *
 * \param  None
 * \return int 正常不会返回
 */
int main(void)
{
	uint32_t ms_elapsed = 0u;
	uint32_t tick_count = 0u;
	uint8_t ch;

	bsp_sysclk_init();   /* 内部 RCH × 12 PLL → 48MHz */
	bsp_led_init();      /* 板载 LED */
	bsp_uart0_init();    /* 调试串口(也是 IAP 触发入口) */

	app_print_banner();

	/* 10ms 心跳主循环:兼顾 LED 周期、打印与控制台轮询 */
	for (;;) {
		delay1ms(APP_HEARTBEAT_MS);

		while (bsp_uart0_get_char(&ch) != 0) {
			app_console_input(ch);
		}

		ms_elapsed += APP_HEARTBEAT_MS;
		if (ms_elapsed >= APP_BLINK_PERIOD_MS) {
			ms_elapsed = 0u;
			tick_count++;
			bsp_led_toggle();
			bsp_uart0_printf("tick %u: LED toggled (sysclk %u Hz)\r\n",
					 (unsigned int)tick_count,
					 (unsigned int)SystemCoreClock);
		}
	}
}
