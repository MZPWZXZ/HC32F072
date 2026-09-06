/**
 * \file bsp_sysclk.c
 * \brief 系统时钟板级驱动实现(无外部晶振方案)
 */
#include "bsp_sysclk.h"

#include "ddl.h"        /* Sysctrl_* 系列接口 */
#include "flash.h"      /* Flash_WaitCycle()  */
#include "system_hc32f072.h" /* SystemCoreClock  */

void bsp_sysclk_init(void)
{
	stc_sysctrl_pll_cfg_t stc_pll_cfg;

	/* 1. 分频:HCLK = PCLK = 系统时钟,1 分频 */
	Sysctrl_SetHCLKDiv(SysctrlHclkDiv1);
	Sysctrl_SetPCLKDiv(SysctrlPclkDiv1);

	/* 2. 切到内部低速 RCL:重新配置 RCH 频率前必须先脱离 RCH */
	Sysctrl_SetRCLTrim(SysctrlRclFreq32768);
	Sysctrl_SetRCLStableTime(SysctrlRclStableCycle64);
	Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);
	Sysctrl_SysClkSwitch(SysctrlClkRCL);

	/* 3. RCH 配为 4MHz 并打开,作为 PLL 输入 */
	Sysctrl_SetRCHTrim(SysctrlRchFreq4MHz);
	Sysctrl_ClkSourceEnable(SysctrlClkRCH, TRUE);

	/* 4. PLL 配置:输入 RCH 4MHz,×12 输出 48MHz */
	DDL_ZERO_STRUCT(stc_pll_cfg);
	stc_pll_cfg.enInFreq    = SysctrlPllInFreq4_6MHz;   /* 4~6MHz 输入档 */
	stc_pll_cfg.enOutFreq   = SysctrlPllOutFreq36_48MHz; /* 输出档       */
	stc_pll_cfg.enPllClkSrc = SysctrlPllRch;             /* 输入源:RCH   */
	stc_pll_cfg.enPllMul    = SysctrlPllMul12;           /* 4MHz × 12    */
	Sysctrl_SetPLLFreq(&stc_pll_cfg);

	/* 5. HCLK > 24MHz,Flash 读等待至少 1 周期(必须在切换前完成) */
	Flash_WaitCycle(FlashWaitCycle1);

	/* 6. 打开 PLL 并切换系统时钟,函数内部会刷新 SystemCoreClock */
	Sysctrl_ClkSourceEnable(SysctrlClkPLL, TRUE);
	Sysctrl_SysClkSwitch(SysctrlClkPLL);
}
