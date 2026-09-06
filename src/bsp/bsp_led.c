/**
 * \file bsp_led.c
 * \brief 板载 LED 驱动实现
 */
#include "bsp_led.h"

#include "board.h"
#include "ddl.h"     /* 基础类型 / delay 等 */
#include "gpio.h"    /* Gpio_* 接口 */
#include "sysctrl.h" /* Sysctrl_SetPeripheralGate */

/**
 * \brief 设置 LED 引脚物理电平
 *
 * \param  level 引脚电平:0 低电平,1 高电平
 * \return None
 */
static void bsp_led_set_level(uint8_t level)
{
	if (level != 0u)
		Gpio_SetIO(BSP_LED1_PORT, BSP_LED1_PIN);
	else
		Gpio_ClrIO(BSP_LED1_PORT, BSP_LED1_PIN);
}

void bsp_led_init(void)
{
	stc_gpio_cfg_t stc_gpio_cfg;

	Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

	DDL_ZERO_STRUCT(stc_gpio_cfg);
	stc_gpio_cfg.enDir = GpioDirOut;
	Gpio_Init(BSP_LED1_PORT, BSP_LED1_PIN, &stc_gpio_cfg);

	bsp_led_off();   /* 初始状态:熄灭 */
}

void bsp_led_on(void)
{
	bsp_led_set_level((BSP_LED_ACTIVE_HIGH != 0u) ? 1u : 0u);
}

void bsp_led_off(void)
{
	bsp_led_set_level((BSP_LED_ACTIVE_HIGH != 0u) ? 0u : 1u);
}

void bsp_led_toggle(void)
{
	if (Gpio_ReadOutputIO(BSP_LED1_PORT, BSP_LED1_PIN) == TRUE)
		bsp_led_set_level(0u);
	else
		bsp_led_set_level(1u);
}
