/**
 * \file startup_hc32f072.S
 * \brief HC32F072(Cortex-M0+)GCC 启动文件
 *
 * 内容:
 *   1. 中断向量表(与官方 DDL 的 EWARM/MDK 启动文件一一对应);
 *   2. Reset_Handler:设置栈指针与 VTOR、拷贝 .data、清零 .bss、
 *      调用 SystemInit() 后进入 main();
 *   3. 兜底弱中断处理函数(Default_Handler 等)。
 *
 * 向量表顺序以官方 DDL Rev1.1.1 的 startup_hc32f072.s 为准;
 * 大部分外设中断处理函数由 mcu/common/interrupts_hc32f072.c 强定义。
 *
 * 说明:本文件使用 GCC 汇编语法(.syntax unified / .thumb)。
 */

    .syntax unified
    .cpu   cortex-m0plus
    .thumb

    .section .isr_vector, "a", %progbits
    .align  8
    .globl  g_pfnVectors
    .type   g_pfnVectors, %object
g_pfnVectors:
    .word   _estack                         /* 0  : 栈顶 */
    .word   Reset_Handler                   /* 1  : 复位 */
    .word   NMI_Handler                     /* 2  : NMI */
    .word   HardFault_Handler               /* 3  : 硬故障 */
    .word   0                               /* 4  : 保留 */
    .word   0                               /* 5  : 保留 */
    .word   0                               /* 6  : 保留 */
    .word   0                               /* 7  : 保留 */
    .word   0                               /* 8  : 保留 */
    .word   0                               /* 9  : 保留 */
    .word   0                               /* 10 : 保留 */
    .word   SVC_Handler                     /* 11 : SVC 调用 */
    .word   0                               /* 12 : 保留 */
    .word   0                               /* 13 : 保留 */
    .word   PendSV_Handler                  /* 14 : PendSV */
    .word   SysTick_Handler                 /* 15 : SysTick */
    /* ---- 外设中断(IRQ0..IRQ31) ---- */
    .word   PORTA_IRQHandler                /* 16 : IRQ0  PORTA */
    .word   PORTB_IRQHandler                /* 17 : IRQ1  PORTB */
    .word   PORTC_E_IRQHandler              /* 18 : IRQ2  PORTC/E */
    .word   PORTD_F_IRQHandler              /* 19 : IRQ3  PORTD/F */
    .word   DMAC_IRQHandler                 /* 20 : IRQ4  DMAC */
    .word   TIM3_IRQHandler                 /* 21 : IRQ5  TIM3 */
    .word   UART0_2_IRQHandler              /* 22 : IRQ6  UART0/2 */
    .word   UART1_3_IRQHandler              /* 23 : IRQ7  UART1/3 */
    .word   LPUART0_IRQHandler              /* 24 : IRQ8  LPUART0 */
    .word   LPUART1_IRQHandler              /* 25 : IRQ9  LPUART1 */
    .word   SPI0_I2S0_IRQHandler            /* 26 : IRQ10 SPI0/I2S0 */
    .word   SPI1_I2S1_IRQHandler            /* 27 : IRQ11 SPI1/I2S1 */
    .word   I2C0_IRQHandler                 /* 28 : IRQ12 I2C0 */
    .word   I2C1_IRQHandler                 /* 29 : IRQ13 I2C1 */
    .word   TIM0_IRQHandler                 /* 30 : IRQ14 TIM0 */
    .word   TIM1_IRQHandler                 /* 31 : IRQ15 TIM1 */
    .word   TIM2_IRQHandler                 /* 32 : IRQ16 TIM2 */
    .word   Default_Handler                 /* 33 : IRQ17 保留 */
    .word   TIM4_IRQHandler                 /* 34 : IRQ18 TIM4 */
    .word   TIM5_IRQHandler                 /* 35 : IRQ19 TIM5 */
    .word   TIM6_IRQHandler                 /* 36 : IRQ20 TIM6 */
    .word   PCA_IRQHandler                  /* 37 : IRQ21 PCA */
    .word   WDT_IRQHandler                  /* 38 : IRQ22 WDT */
    .word   Default_Handler                 /* 39 : IRQ23 保留 */
    .word   ADC_DAC_IRQHandler              /* 40 : IRQ24 ADC/DAC */
    .word   Default_Handler                 /* 41 : IRQ25 保留 */
    .word   VC0_1_2_LVD_IRQHandler          /* 42 : IRQ26 VC0/1/2/LVD */
    .word   USBFS_IRQHandler                /* 43 : IRQ27 USBFS */
    .word   CAN_IRQHandler                  /* 44 : IRQ28 CAN */
    .word   Default_Handler                 /* 45 : IRQ29 保留 */
    .word   FLASH_RAM_IRQHandler            /* 46 : IRQ30 FLASH/RAM */
    .word   CLKTRIM_CTS_IRQHandler          /* 47 : IRQ31 CLKTRIM/CTS */
    .size   g_pfnVectors, . - g_pfnVectors

    .section .text, "ax", %progbits

/**
 * \brief 复位处理:芯片上电/复位后的第一条执行路径
 *
 * 流程:初始化栈与向量表 → 拷贝 .data → 清零 .bss →
 *       调用 DDL 的 SystemInit()(设置内部时钟 TRIM 与隐藏引脚)→ main()。
 */
    .thumb_func
    .type   Reset_Handler, %function
    .globl  Reset_Handler
Reset_Handler:
    /* 1. 设置栈指针(Cortex-M0 需经通用寄存器装载) */
    ldr     r0, =_estack
    mov     sp, r0

    /* 2. 设置向量表偏移寄存器 VTOR(SCB 基址 + 0xED08) */
    ldr     r0, =0xE000ED08
    ldr     r1, =_svectors
    str     r1, [r0]

    /* 3. 拷贝 .data:Flash(_sidata) → RAM(_sdata.._edata) */
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_sidata
copy_data:
    cmp     r0, r1
    bhs     copy_data_done
    ldr     r3, [r2]
    str     r3, [r0]
    adds    r0, r0, #4
    adds    r2, r2, #4
    b       copy_data
copy_data_done:

    /* 4. 清零 .bss(_sbss.._ebss) */
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
zero_bss:
    cmp     r0, r1
    bhs     zero_bss_done
    str     r2, [r0]
    adds    r0, r0, #4
    b       zero_bss
zero_bss_done:

    /* 5. 调用 DDL 系统初始化,然后进入 main() */
    ldr     r0, =SystemInit
    blx     r0
    ldr     r0, =main
    blx     r0

    /* main() 意外返回时死循环 */
main_exit:
    b       main_exit
    .size   Reset_Handler, . - Reset_Handler

/* ---------------------------------------------------------------------------
 * 弱中断处理:以下四个异常处理器在启动文件中提供弱定义;
 * 外设中断处理函数由 interrupts_hc32f072.c 强定义,故向量表直接引用。
 * ------------------------------------------------------------------------- */
    .thumb_func
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler                 /* 未实现的中断:原地死循环便于调试 */
    .size   Default_Handler, . - Default_Handler

    .thumb_func
    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    b       Default_Handler
    .size   NMI_Handler, . - NMI_Handler

    .thumb_func
    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    b       Default_Handler
    .size   SVC_Handler, . - SVC_Handler

    .thumb_func
    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    b       Default_Handler
    .size   PendSV_Handler, . - PendSV_Handler

    .end
