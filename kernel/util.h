#ifndef UTIL_H
#define UTIL_H

#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)
#define NULL   ((void*)0)

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(unsigned char *dest, unsigned char val, unsigned long len);
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

