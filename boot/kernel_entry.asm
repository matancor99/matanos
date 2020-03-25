[bits 32]
[extern _start] ; Define calling point. Must have same name as kernel.c 'main' function
[extern _end] ; setting the stuck a bit more close to the end of the kernel.
mov ebp, _end
add ebp, 0x1000
mov esp, ebp
call _start ; Calls the C function. The linker will know where it is placed in memory
jmp $
