ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(512);
        KEEP(*(.isr_vector))
    } > FLASH
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH
    .mkr_manifest :
    {
        . = ALIGN(4);
        KEEP(*(.mkr_manifest))
        . = ALIGN(4);
    } > FLASH
    _etext = .;
    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM
    _sidata = LOADADDR(.data);
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.comment)
    }
}
