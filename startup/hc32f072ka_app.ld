/*
 * HC32F072KA App 镜像链接脚本(供 Bootloader 通过串口 IAP 升级)
 *
 * 布局(见 src/iap_shared.h):
 *   FLASH : App @0x00008000, 96KB(0x00008000 ~ 0x0001FFFF)
 *   RAM   : 16KB,但头部 0x40 字节保留给 IAP 引导标志,
 *           运行区从 0x20000040 开始(与 Bootloader 链接脚本一致)
 *
 * 对应目标:CMake 的 hc32f072ka_app
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00008000, LENGTH = 96K
    RAM   (rwx) : ORIGIN = 0x20000040, LENGTH = 16K - 0x40
}

/* 栈顶 = RAM 可用区顶部(Cortex-M 向下生长的满减栈) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* 中断向量表:App 向量表随镜像位于 0x8000,Reset_Handler 会写 VTOR 指向此处 */
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
