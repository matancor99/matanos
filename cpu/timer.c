#include "timer.h"
#include "screen.h"
#include "util.h"
#include "printf.h"
#include "ports.h"
#include "task.h"
#include "isr.h"

#define TIMER_COMMAND_PORT 0x43
#define TIMER_DATA_PORT 0x40
#define SET_REPEAT_MODE 0x36
unsigned int tick = 0;

extern task_t *current_task;
extern task_t *ready_queue;
extern uint32_t next_pid;
uint32_t end_of_sleep = 0;
uint32_t sleeper_task = 0;


static void timer_callback(registers_t regs) {

        task_t * next_task = NULL;
        tick++;
        // There is only one task
        if (next_pid == 1) {
            return;
        }

        // Reset the sleep task
        if (tick > end_of_sleep) {
            sleeper_task = 0;
        }

        // Set the next_task to run
        if (current_task->next == NULL) {
            next_task = ready_queue;
        }
        else {
            next_task = current_task->next;
        }

        // Switch task only if the next task isn't sleeping
        if(sleeper_task && next_task && next_task->id == sleeper_task) {
//            printf("Sleeping\n");
        }
        else {
//            printf("Not Sleeping %d %d %d\n", sleeper_task, tick, end_of_sleep);
            switch_task();
        }
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


void sleep(uint32_t epoches) {
    if (!sleeper_task) {
        end_of_sleep = tick + epoches;
        sleeper_task = current_task->id;
        //TODO: This is very problematic call since it does not save all the registers
        switch_task();
    }
}
