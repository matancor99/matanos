void memory_copy(char *source, char *dest, int nbytes);
void int_to_ascii(int n, char str[]);


#define low_16(address) (unsigned short)((address) & 0xFFFF)
#define high_16(address) (unsigned short)(((address) >> 16) & 0xFFFF)