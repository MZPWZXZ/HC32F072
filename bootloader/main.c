/**
 * \file bootloader/main.c
 * \brief HC32F072KA 串口 IAP Bootloader 主程序
 *
 * 启动流程:
 *   1. 系统时钟 48MHz(内部 RCH→PLL,与 App 一致);
 *   2. 若 RAM 保留区存在“进入引导”魔数(App 串口命令写入后软复位),
 *      清除魔数并停留在 Bootloader;
 *   3. 否则若 App 区向量表合法,直接跳转到 App;
 *   4. 否则(无 App/App 非法)停留在 Bootloader。
 *
 * 停留期间:LED 慢闪提示;轮询处理主机(上位)发来的 IAP 帧
 * (见 iap_proto 模块),收到合法新 App 后自动跳转运行。
 */
#include "bsp_led.h"
#include "bsp_sysclk.h"
#include "bsp_uart.h"

#include "board.h"           /* BSP_UART0_BAUDRATE */
#include "ddl.h"             /* delay1ms() */
#include "iap_proto.h"
#include "iap_shared.h"
#include "system_hc32f072.h"

/**
 * \brief 检查并清除“进入引导”魔数
 *
 * \param  None
 * \return uint8_t 1 存在魔数(应停留在 Bootloader);0 不存在
 */
static uint8_t boot_enter_requested(void)
{
	volatile uint32_t *p_magic = (volatile uint32_t *)IAP_MAGIC_ADDR;

	if (*p_magic == IAP_MAGIC_WORD) {
		*p_magic = 0u;   /* 清除,避免复位后再次进入 */
		return 1u;
	}

	return 0u;
}

/**
 * \brief 打印 Bootloader 启动信息
 *
 * \param  None
 * \return None
 */
static void boot_print_banner(void)
{
	bsp_uart0_write("\r\n");
	bsp_uart0_write("========================================\r\n");
	bsp_uart0_write(" HC32F072KA Bootloader v1.0\r\n");
	bsp_uart0_printf(" App base : 0x%X  max %u KB\r\n",
			 (unsigned int)IAP_APP_BASE,
			 (unsigned int)(IAP_APP_MAX_SIZE / 1024u));
	bsp_uart0_printf(" SysClk   : %u Hz (internal RC x12 PLL)\r\n",
			 (unsigned int)SystemCoreClock);
	bsp_uart0_printf(" UART0    : %u bps 8N1\r\n",
			 (unsigned int)BSP_UART0_BAUDRATE);
	bsp_uart0_write(" 等待主机 IAP 帧(SYNC/WRITE/DONE)...\r\n");
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
	uint32_t idle_loops = 0u;

	bsp_sysclk_init();
	bsp_led_init();
	bsp_uart0_init();
	iap_init();   /* 按 48MHz 配置 Flash 编程时间参数 */

	if (boot_enter_requested() != 0u) {
		bsp_uart0_write(">> IAP mode requested by app\r\n");
	} else if (iap_app_is_valid() != 0u) {
		bsp_uart0_write(">> valid app found, jump to app ...\r\n");
		delay1ms(100u);
		iap_jump_to_app();
	} else {
		bsp_uart0_write(">> no valid app, stay in bootloader\r\n");
	}

	boot_print_banner();

	/* 停留:每 ~250ms 轮询一帧;LED 约每 500ms 翻转一次 */
	for (;;) {
		if (iap_recv_process(250u) != 0) {
			idle_loops = 0u;   /* 刚处理过命令,LED 快速闪提示?复位计数 */
			continue;
		}
		if ((idle_loops++ & 1u) == 0u) {
			bsp_led_toggle();
		}
	}
}
