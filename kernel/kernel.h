
#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>
extern uint32_t end;
extern uint32_t code;
extern uint32_t data;
extern uint32_t data_end;
extern uint32_t bss;
extern uint32_t sector_num;
void user_input(char *input);

#endif