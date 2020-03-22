#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"


void main() {

    //Initializing all the processor interrupt related structures
    isr_install();
    IRQ_set_mask(CLOCK_INTERRUPT_LINE);
//    IRQ_set_mask(KEYBOARD_INTERRUPT_LINE);
    // Timer interrupt at 50HZ
    init_timer(50);
    // Keyboard interrupt
    init_keyboard();
}

void _start() {
    main();
}

