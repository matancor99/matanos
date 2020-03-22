#include "timer.h"
#include "../drivers/screen.h"
#include "../kernel/util.h"
#include "../drivers/ports.h"
#include "isr.h"

#define TIMER_COMMAND_PORT 0x43
#define TIMER_DATA_PORT 0x40
#define SET_REPEAT_MODE 0x36
unsigned int tick = 0;

static void timer_callback(registers_t regs) {
    tick++;
    kprint("Tick: ");
    
    char tick_ascii[256];
    int_to_ascii(tick, tick_ascii);
    kprint(tick_ascii);
    kprint("\n");
}

void init_timer(unsigned int freq) {
    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    unsigned int divisor = 1193180 / freq;
    unsigned char low  = (unsigned char)(divisor & 0xFF);
    unsigned char high = (unsigned char)( (divisor >> 8) & 0xFF);
    /* Send the command */
    port_byte_out(TIMER_COMMAND_PORT, SET_REPEAT_MODE); /* Command port */
    port_byte_out(TIMER_DATA_PORT, low);
    port_byte_out(TIMER_DATA_PORT, high);
}

