/*
 * HC32F072KA Bootloader 镜像链接脚本
 *
 * 布局(见 src/iap_shared.h):
 *   FLASH : Bootloader @0x00000000, 32KB(0x00000000 ~ 0x00007FFF)
 *   RAM   : 16KB,但头部 0x40 字节保留给 IAP 引导标志,
 *           运行区从 0x20000040 开始(与 App 链接脚本一致)
 *
 * 注意:HC32F072(>32KB Flash)要求 Flash 擦写代码运行在 0~32K 区域,
 *       本镜像整体位于 0x0~0x7FFF,满足该约束。
 *
 * 对应目标:CMake 的 hc32f072ka_boot
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
    RAM   (rwx) : ORIGIN = 0x20000040, LENGTH = 16K - 0x40
}

/* 栈顶 = RAM 可用区顶部(Cortex-M 向下生长的满减栈) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* 中断向量表:复位后从 Flash 0x0 启动,Reset_Handler 写 VTOR=_svectors */
    .isr_vector :
    {
        . = ALIGN(8);
        _svectors = .;
        KEEP(*(.isr_vector))
        . = ALIGN(4);
        _evectors = .;
    } > FLASH

    /* 代码与只读数据 */
    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 已初始化数据:运行期位于 RAM,初始值存放在 Flash(_sidata) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH
    _sidata = LOADADDR(.data);

    /* 零初始化数据 */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* 堆边界符号(当前工程未使用堆,详见 main 分支的说明) */
    . = ALIGN(8);
    _heap_start = .;
    _heap_end   = _estack;

    /* 兼容 newlib 的结束符号 */
    _end = .;
    PROVIDE(end = .);

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.eh_frame*)
    }
}
