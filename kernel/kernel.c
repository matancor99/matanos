#include "isr.h"
#include "timer.h"
#include "paging.h"
#include "kheap.h"
#include "task.h"
#include "syscall.h"
#include "keyboard.h"
#include "screen.h"
#include "ports.h"
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


void do_user_mode() {

    initialise_syscalls();

    switch_to_user_mode();

    syscall_print_string("Running in user mode!\n");
}

void general_fault(registers_t regs)
{
    printf("Error flag is 0x%08x \n", regs.err_code);
    PANIC("General fault");
}


// Somehow the time between the two processes is imbalanced idk why.

// For some reason i cant have process both in user node and in kernel mode + in user mode irq's not working.
void do_tasking_test() {
    int ret = fork();
    printf("passed fork ret %d\n", ret);
    int do_nothing_iterations = 1000000;
    int print_iterations = 3;
    for (int i=0; i<print_iterations; i++) {
        if (ret >= 0) {
            for (int j = 0; j < do_nothing_iterations; j++) {
                int x = 0;
                x += 9999 / 9;
            }
        }
        printf("fork() returned %d, and getpid() returned %d\n", ret, getpid());
    }

//    printf("fork() returned %d, and getpid() returned %d\n", ret, getpid());
//    if (ret > 0) {   //parent
//        for(;;) {
//            printf("Running in kernel mode %d\n", a);
//        }
//    }
//    else {   //child
//        printf("Running in kernel mode %d\n", b);
//        do_user_mode();
//    }
}

void main() {

    A20_sanity_checks();
    //Initializing all the processor interrupt related structures
    init_gdt();

    isr_install();
//    IRQ_set_mask(CLOCK_INTERRUPT_LINE);
//    IRQ_set_mask(KEYBOARD_INTERRUPT_LINE);
    // Timer interrupt at 50HZ
    init_timer(50);
    // Keyboard interrupt
    init_keyboard();

    register_interrupt_handler(13, general_fault);


    initialise_paging();

    // Start multitasking.
    initialise_tasking();

    // Create a new process in a new address space which is a clone of this.
//    do_tasking_test();
    do_user_mode();

    for(;;);
}

void _start() {
    main();
    for(;;);
}

