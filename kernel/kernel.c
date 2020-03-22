#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "printf.h"

extern int is_A20_on();

void A20_sanity_checks(){
    // Handle A20
    init_A20();
    unsigned char * addr = 0x011111;
    unsigned char * addr2 = 0x111111;
    *addr = 0x1;
    *addr2 = 0x2;
    if (*addr == *addr2 ) {
        kprint("Equal addresses\n");
    }
    else {
        kprint("Not Equal addresses\n");
    }

    if (is_A20_on()) {
        kprint("A20_on\n");
    }
    else {
        kprint("A20_off\n");
    }
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    kprint("You said: ");
    kprint(input);
    kprint("\n> ");
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

    int x  = 3;
    printf("Im the king %d", x);

}

void _start() {
    main();
}

