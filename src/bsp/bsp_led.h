/**
 * \file bsp_led.h
 * \brief 板载 LED 驱动接口
 *
 * 引脚与点亮电平见 board.h(BSP_LED1_PORT/PIN、BSP_LED_ACTIVE_HIGH)。
 */
#ifndef BSP_BSP_LED_H
#define BSP_BSP_LED_H

/**
 * \brief 初始化 LED 引脚为推挽输出,并置为熄灭电平
 *
 * \param  None
 * \return None
 */
void bsp_led_init(void);

/**
 * \brief 点亮 LED(按 BSP_LED_ACTIVE_HIGH 处理电平)
 *
 * \param  None
 * \return None
 */
void bsp_led_on(void);

/**
 * \brief 熄灭 LED(按 BSP_LED_ACTIVE_HIGH 处理电平)
 *
 * \param  None
 * \return None
 */
void bsp_led_off(void);

/**
 * \brief 翻转 LED 状态
 *
 * \param  None
 * \return None
 */
void bsp_led_toggle(void);

#endif /* BSP_BSP_LED_H */
