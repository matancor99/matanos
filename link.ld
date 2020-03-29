extra_sector_num = 15;

ENTRY(_start)
STARTUP(boot/kernel_entry.o)
SECTIONS
{
    .boot 0x7c00 :         /*for the boot section the lma = vma - both will start the binary*/
    {
       *(.boot_text)
    }

    .text 0x8000 : AT ( ADDR (.boot) + SIZEOF (.boot) )   /* leave a place for a small stack at 0x8000 - 0x7e00 -
    the text case things are a little different, it is places excetly after the boot in ROM but not in RAM*/
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
        data_end = .;
    }

    . += extra_sector_num * 512;
    . = ALIGN(4096);
    
    .bss :
    {
        bss = .; _bss = .; __bss = .;
        *(.bss)
        . = ALIGN(4096);
    }
    end = .; _end = .; __end = .;
    sector_num = (data_end - _code) / 512 + 1 + extra_sector_num;
}
