#ifndef UTIL_H
#define UTIL_H

#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(unsigned char *dest, unsigned char val, unsigned long len);
void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);

#endif

