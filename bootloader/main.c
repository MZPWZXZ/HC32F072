/**
 * \file bootloader/main.c
 * \brief HC32F072KA 串口 IAP Bootloader 主程序(最小骨架)
 *
 * 启动流程:
 *   1. 系统时钟 48MHz(内部 RCH→PLL,与 App 一致);
 *   2. 若 RAM 保留区存在“进入引导”魔数(由 App 串口命令写入后软复位),
 *      清除魔数并停留在 Bootloader;
 *   3. 否则若 App 区向量表合法,直接跳转到 App;
 *   4. 否则(无 App/App 非法)停留在 Bootloader。
 *
 * 停留期间闪烁 LED 并等待上位机;串口 IAP 协议处理见 iap_proto 模块。
 */
#include "bsp_led.h"
#include "bsp_sysclk.h"
#include "bsp_uart.h"

#include "board.h"           /* BSP_UART0_BAUDRATE */
#include "ddl.h"             /* delay1ms() */
#include "iap_shared.h"
#include "system_hc32f072.h"

/**
 * \brief 判断 App 区向量表是否合法
 *
 * 校验条件(App 区被擦除时数据为 0xFF,自然判为非法):
 *   - 初始 SP 落在 SRAM 范围内;
 *   - 复位向量(bit0=1,Thumb)落在 App 区段内。
 *
 * \param  None
 * \return uint8_t 1 合法;0 非法
 */
static uint8_t boot_app_is_valid(void)
{
	uint32_t sp;
	uint32_t pc;

	sp = *(volatile uint32_t *)(IAP_APP_BASE);
	pc = *(volatile uint32_t *)(IAP_APP_BASE + 4u);

	if ((sp < IAP_RAM_BASE) || (sp >= IAP_RAM_BASE + 0x4000u)) {
		return 0u;
	}

	if ((pc & 1u) == 0u) {
		return 0u;   /* 非 Thumb */
	}
	pc &= ~1u;
	if ((pc < IAP_APP_BASE) ||
	    (pc >= IAP_APP_BASE + IAP_APP_MAX_SIZE)) {
		return 0u;
	}

	return 1u;
}

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
 * \brief 跳转到 App(入口为 App 区向量表的复位向量)
 *
 * App 的 Reset_Handler 会自行重设栈、VTOR 并拷贝 .data,因此这里
 * 只需按 CMSIS 惯例设置 MSP 后跳转。
 *
 * \param  None
 * \return None
 */
static void boot_jump_to_app(void)
{
	uint32_t pc;

	pc = *(volatile uint32_t *)(IAP_APP_BASE + 4u);

	__disable_irq();
	__set_MSP(*(volatile uint32_t *)IAP_APP_BASE);
	__DSB();
	__ISB();

	((void (*)(void))pc)();   /* pc 含 Thumb 位 */

	for (;;) {
		;   /* 正常不会到达 */
	}
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
	bsp_sysclk_init();
	bsp_led_init();
	bsp_uart0_init();

	if (boot_enter_requested() != 0u) {
		bsp_uart0_write(">> IAP mode requested by app\r\n");
	} else if (boot_app_is_valid() != 0u) {
		bsp_uart0_write(">> valid app found, jump to app ...\r\n");
		boot_jump_to_app();
	} else {
		bsp_uart0_write(">> no valid app, stay in bootloader\r\n");
	}

	boot_print_banner();

	/* 停留:闪烁 LED 提示处于引导模式(协议处理在后续步骤接入) */
	for (;;) {
		bsp_led_toggle();
		delay1ms(500u);
	}
}
