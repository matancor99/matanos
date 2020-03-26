#ifndef PRINTF_H
#define PRINTF_H

#include <stdint.h>
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
void print_nice_hex(uint32_t * addr, int num);

#endif