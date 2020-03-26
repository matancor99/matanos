#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdbool.h>
#include <stdint.h>

struct symbol_table_header {
    uint32_t symbol_table_size;
    uint32_t str_table_size;
};


typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	// Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

typedef struct {
    Elf32_Word		st_name;
    Elf32_Addr		st_value;
    Elf32_Word		st_size;
    uint8_t			st_info;
    uint8_t			st_other;
    Elf32_Half		st_shndx;
} Elf32_Sym;

void parse_symbol_table();
bool find_symbol_and_run(Elf32_Sym * symbol_table, char * str_table, uint32_t sym_num, char * symname, uint32_t param1, uint32_t param2, uint32_t param3);

#endif