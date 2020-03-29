#ifndef UTIL_H
#define UTIL_H

#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)
#define NULL   ((void*)0)

#include "printf.h"

#define static_assert(expr, msg) _Static_assert(expr, msg)
#define static_assert_sizeof(type, size) \
    static_assert(sizeof(type) == size, "sizeof(" #type ") is not equal to " #size)

void panic(const char *message, const char *file, int line);
#define PANIC(msg) panic(msg, __FILE__, __LINE__);

void panic_assert(const char *file, int line, const char *desc);
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))

void memcpy(char *source, char *dest, int nbytes);
void memset(unsigned char *dest, unsigned char val, unsigned long len);
void int_to_ascii(int n, char str[]);
void hex_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
int strnlen(char s[], int n);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);
/* Sometimes we want to keep parameters to a function for later use
 * and this is a solution to avoid the 'unused parameter' compiler warning */
#define UNUSED(x) (void)(x)

#endif

