; Identical to lesson 13's boot sector, but the %included files have new paths
SECTION .boot_text

[extern realmode_stack]
[bits 16]
genesis:
    mov [BOOT_DRIVE], dl ; Remember that the BIOS sets us the boot drive in 'dl' on boot
    mov bp, realmode_stack ; arbitrary stack location for real mode
    mov sp, bp

    mov bx, MSG_REAL_MODE 
    call print
    call print_nl

    call load_kernel ; read the kernel from disk
    call switch_to_pm ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
    jmp $ ; Never executed

%include "/root/CLionProjects/matanos/boot/print.asm"   ; The abs path is for the cmake support
%include "/root/CLionProjects/matanos/boot/print_hex.asm"
%include "/root/CLionProjects/matanos/boot/disk.asm"
%include "/root/CLionProjects/matanos/boot/gdt.asm"
%include "/root/CLionProjects/matanos/boot/32bit_print.asm"
%include "/root/CLionProjects/matanos/boot/switch_pm.asm"

[extern _code]
[extern sector_num]
[bits 16]
load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print
    call print_nl

    mov bx, _code ; Read from disk and store in the linker script .text address

    mov dh, sector_num ; Our future kernel will be larger, make this big
    mov dl, [BOOT_DRIVE]
    call disk_load
    ret

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    call _code ; Give control to the kernel
    jmp $ ; Stay here when the kernel returns control to us (if ever)


BOOT_DRIVE db 0 ; It is a good idea to store it in memory because 'dl' may get overwritten
MSG_REAL_MODE db "Started in 16-bit Real Mode", 0
MSG_PROT_MODE db "Landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0

; padding
times 510 - ($-$$) db 0
dw 0xaa55
