/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
/*            Original file taken from Bran's Kernel Development */
/*            tutorials: http://www.osdever.net/bkerndev/index.php. */

ENTRY(_start)
STARTUP(boot/kernel_entry.o)
SECTIONS
{
    .boot 0x7c00 :         /*for the boot section the lma = vma*/
    {
       *(.boot_text)
    }

    .text 0x8000 : AT ( ADDR (.boot) + SIZEOF (.boot) )   /* leave a place for a small stack at 0x8000 - 0x7e00*/
    {
        code = .; _code = .; __code = .; realmode_stack = .;
        *(.text)
        . = ALIGN(4096);
    }

    .data :
    {
        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }

    .bss :
    {
        bss = .; _bss = .; __bss = .;
        *(.bss)
        . = ALIGN(4096);
    }
    end = .; _end = .; __end = .;
    sector_num = (end - _code) / 512;
}
