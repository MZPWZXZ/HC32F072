/**
 * \file board.h
 * \brief 板级资源集中配置(引脚/电平/外设参数)
 *
 * 不同硬件板的 LED、串口引脚可能不同,修改本文件即可,无需改动驱动代码。
 * 注意:引脚复用功能(AF)编号与 HC32F072 数据手册的 IO 复用表一致。
 */
#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include "gpio.h"

/* ===========================================================================
 * LED1:板载指示 LED(默认占位引脚,请按实际原理图修改)
 * ========================================================================= */
#define BSP_LED1_PORT          GpioPortC   /* LED 所在 GPIO 端口          */
#define BSP_LED1_PIN           GpioPin13   /* LED 所在引脚(默认占位 PC13) */
#define BSP_LED_ACTIVE_HIGH    (1u)        /* 1:高电平点亮;0:低电平点亮   */

/* ===========================================================================
 * UART0:调试串口(默认 115200-8-N-1)
 * 引脚取自官方 DDL 64PIN 例程:PA09=UART0_TXD(AF1),PA10=UART0_RXD(AF1)
 * ========================================================================= */
#define BSP_UART0_TX_PORT      GpioPortA
#define BSP_UART0_TX_PIN       GpioPin9
#define BSP_UART0_TX_AF        GpioAf1
#define BSP_UART0_RX_PORT      GpioPortA
#define BSP_UART0_RX_PIN       GpioPin10
#define BSP_UART0_RX_AF        GpioAf1
#define BSP_UART0_BAUDRATE     (115200u)

#endif /* BSP_BOARD_H */
