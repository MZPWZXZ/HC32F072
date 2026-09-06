/*
 * HC32F072KA 链接脚本
 *
 * 存储布局(来自 HC32F072 数据手册与官方 DDL 模板):
 *   FLASH: 128KB,起始 0x00000000
 *   SRAM : 16KB, 起始 0x20000000
 *
 * 无外部晶振、无 bootloader,向量表直接从 Flash 0x0 启动。
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 128K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

/* 栈顶 = RAM 最高地址(Cortex-M 向下生长的满减栈) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* 中断向量表,固定放在 Flash 起始,见 startup/startup_hc32f072.S */
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

    /* 堆边界符号(当前工程未提供 _sbrk,newlib-nano 路径不使用堆;
     * 若后续引入 malloc 系调用,需自行实现 _sbrk 并限定在 [_heap_start, _heap_end)) */
    . = ALIGN(8);
    _heap_start = .;
    _heap_end   = _estack;

    /* 兼容 newlib 的结束符号 */
    _end = .;
    PROVIDE(end = .);

    /* 丢弃无需保留的异常展开/栈回溯表(裸机不使用) */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.eh_frame*)
    }
}
