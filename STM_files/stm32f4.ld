/* Define Firmware's entry point */
ENTRY(Reset_Handler)

/* Defining memory sections */
MEMORY
{
    FLASH(rx) : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Defining Top of the Stack */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/**/
SECTIONS 
{
    .text : 
    {
        . = ALIGN(4);
        *(.isr_vector_tbl)
        *(.text)
        *(.rodata)
        . = ALIGN(4);
        _etext = .;
    } >FLASH
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        . = ALIGN(4);
        _edata = .;
    } >SRAM AT> FLASH /* execute in VMA, store at LMA */
   .bss :
   {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        . = ALIGN(4);
        _ebss = .;
   } >SRAM
}