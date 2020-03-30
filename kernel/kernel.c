#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../cpu/kheap.h"
#include "../cpu/task.h"
#include "../cpu/syscall.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "printf.h"
#include "symbols.h"
#include "kernel.h"
extern uint32_t next_pid;
extern int is_A20_on();

void A20_sanity_checks(){
    // Handle A20
    init_A20();
    unsigned char * addr = 0x011111;
    unsigned char * addr2 = 0x111111;
    *addr = 0x1;
    *addr2 = 0x2;
    if (*addr == *addr2 ) {
        printf("Equal addresses\n");
    }
    else {
        printf("Not Equal addresses\n");
    }

    if (is_A20_on()) {
        printf("A20_on\n");
    }
    else {
        printf("A20_off\n");
    }
}

void general_fault(registers_t regs)
{
    printf("Error flag is 0x%08x \n", regs.err_code);
    PANIC("General fault");
}


void main() {

    A20_sanity_checks();
    //Initializing all the processor interrupt related structures
    init_gdt();

    isr_install();
    IRQ_set_mask(CLOCK_INTERRUPT_LINE);
//    IRQ_set_mask(KEYBOARD_INTERRUPT_LINE);
    // Timer interrupt at 50HZ
    init_timer(50);
    // Keyboard interrupt
    init_keyboard();

//    printf("The sector_num of the kernel is %d\n", (uint32_t)&sector_num);
//    printf("The data_end of the kernel is 0x%08x\n", (uint32_t)&data_end);
//    printf("The bss of the kernel is 0x%08x\n", (uint32_t)&bss);
//    printf("The end of the kernel is 0x%08x\n", (uint32_t)&end);

    initialise_paging();


    // Start multitasking.
    initialise_tasking();

    // Create a new process in a new address space which is a clone of this.

    // Somehow the time between the two processes is imbalanced idk why.
//    int ret = fork();
//    for (int i=0; i<10; i++) {
//        for (int j=0; j<100000000; j++) {
//            int x = 0;
//            x += 9999/9;
//        }
//        printf("fork() returned %d, and getpid() returned %d\n", ret, getpid());
//    }

    initialise_syscalls();

    register_interrupt_handler(13, general_fault);

    switch_to_user_mode();

    syscall_print_string("Hello, user world!\n");
//    printf("Hello, user world!\n");

    for(;;);
}

void _start() {
    main();
    for(;;);
}

