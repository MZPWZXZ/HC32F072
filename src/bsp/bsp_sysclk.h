/**
 * \file bsp_sysclk.h
 * \brief 系统时钟板级初始化接口
 *
 * 芯片未外接晶振,时钟源全部来自内部 RC:
 *   内部高速 RCH 4MHz → PLL ×12 → 48MHz(HCLK = PCLK = 48MHz)。
 */
#ifndef BSP_BSP_SYSCLK_H
#define BSP_BSP_SYSCLK_H

#include <stdint.h>

/**
 * \brief 系统时钟初始化(内部 RCH → PLL 48MHz)
 *
 * 流程(参照官方 DDL 时钟切换例程):
 *   1. HCLK/PCLK 置 1 分频;
 *   2. 先把系统时钟切到内部低速 RCL,便于安全重配 RCH;
 *   3. RCH 配为 4MHz 并打开;
 *   4. PLL 配为 RCH 4MHz × 12 = 48MHz;
 *   5. HCLK > 24MHz,Flash 读等待置 1 周期;
 *   6. 打开 PLL 并将系统时钟切到 PLL。
 *
 * 成功后 SystemCoreClock 会被刷新为 48MHz(见 system_hc32f072.h)。
 *
 * \param  None
 * \return None
 */
void bsp_sysclk_init(void);

#endif /* BSP_BSP_SYSCLK_H */
