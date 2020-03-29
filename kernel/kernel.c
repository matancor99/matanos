#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../cpu/kheap.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "printf.h"
#include "symbols.h"
#include "kernel.h"

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


void main() {
    A20_sanity_checks();
    //Initializing all the processor interrupt related structures
    isr_install();
    IRQ_set_mask(CLOCK_INTERRUPT_LINE);
//    IRQ_set_mask(KEYBOARD_INTERRUPT_LINE);
    // Timer interrupt at 50HZ
    init_timer(50);
    // Keyboard interrupt
    init_keyboard();
    printf("The sector_num of the kernel is %d\n", (uint32_t)&sector_num);

    uint32_t a = kmalloc(8);
    initialise_paging();
    uint32_t b = kmalloc(8);
    uint32_t c = kmalloc(8);
    printf("a: 0x%08x, b: 0x%08x, c: 0x%08x \n", a, b, c);
    kfree(c);
    kfree(b);
    uint32_t d = kmalloc(12);
    printf("d: 0x%08x,\n", d);

}

void _start() {
    main();
}

