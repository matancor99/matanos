#include "timer.h"
#include "screen.h"
#include "util.h"
#include "printf.h"
#include "ports.h"
#include "task.h"
#include "isr.h"
#include <stdbool.h>

#define TIMER_COMMAND_PORT 0x43
#define TIMER_DATA_PORT 0x40
#define SET_REPEAT_MODE 0x36
unsigned int tick = 0;

extern task_t *current_task;
extern task_t *ready_queue;
extern task_t *sleep_queue;
extern uint32_t next_pid;

static void timer_callback(registers_t regs) {

        tick++;
        // There is only one task
        if (next_pid == 1) {
            return;
        }
        // Reset the sleep task
        for (task_t *tmp_task = (task_t*)sleep_queue; tmp_task; tmp_task = tmp_task->next){
            if (tick > tmp_task->end_of_sleep) {
                tmp_task->end_of_sleep = 0;
                //Remove from sleep queue
                //If tmp_task starts the list
                if (tmp_task == sleep_queue) {
                    sleep_queue = tmp_task->next;
                }
                else {
                    //Find the pointer in the list that points at current task
                    task_t *tmp_task_iter = (task_t*)sleep_queue;
                    while (tmp_task_iter->next != tmp_task) {
                        tmp_task_iter = tmp_task_iter->next;
                    }
                    // Skip current task
                    tmp_task_iter->next = tmp_task->next;
                }

                //Add to ready queue
                task_t *tmp_task_iter = (task_t*)ready_queue;
                while (tmp_task_iter->next)
                    tmp_task_iter = tmp_task_iter->next;
                tmp_task_iter->next = tmp_task;
                tmp_task->next = NULL;
            }
        }

        // Switch task only if the next task isn't sleeping
        switch_task();
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

    //Sanity check - make sure there is more than one task in ready queue else return with error
    if (current_task && current_task == ready_queue && ready_queue->next == NULL) {
        printf("Cant sleep with only one process\n");
        return;
    }

    current_task->end_of_sleep = tick + epoches;
    //If sleep queue is empty - create it
    if(!sleep_queue) {
        sleep_queue = current_task;
        //current_task->next = NULL;
    }
    else {
        // Add it to the end of the ready queue.
        task_t *tmp_task = sleep_queue;
        while (tmp_task->next)
            tmp_task = tmp_task->next;
        tmp_task->next = current_task;
    }

    //TODO: This is very problematic call since it does not save all the registers
    switch_task();
}
