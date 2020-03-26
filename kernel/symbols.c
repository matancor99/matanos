#include "symbols.h"
#include "printf.h"
#include "kernel.h"
#include "util.h"

typedef void (*my_func)();

void print_symbol_table(Elf32_Sym * symbol_table, char * str_table,  uint32_t start, uint32_t end) {
    int i = start;
    char * symbol_name = NULL;
    Elf32_Sym symbol = {0};
    for (i = start; i < end; i++) {
        symbol = symbol_table[i];
        symbol_name = str_table + symbol.st_name;
        printf("Symbol info: name: %s, addr: 0x%08x\n", symbol_name, symbol.st_value);
    }
}

bool find_symbol_and_run(Elf32_Sym * symbol_table, char * str_table, uint32_t sym_num, char * symname, uint32_t param1, uint32_t param2, uint32_t param3) {
    int i = 0;
    bool ret = false;
    char * symbol_name = NULL;
    Elf32_Sym symbol = {0};
    for (i = 0; i < sym_num; i++) {
        symbol = symbol_table[i];
        symbol_name = str_table + symbol.st_name;
        if (strcmp(symbol_name, symname) == 0) {
            my_func func = (my_func)symbol.st_value;
            if (func != 0) {
                func(param1, param2, param3);
                ret = true;
            }
            else {
                printf("Null function\n");
            }
            break;
        }
    }
    if (ret == false) {
        printf("Symbol not found\n");
    }
    return ret;
}

void parse_symbol_table() {
    struct symbol_table_header * header = (struct symbol_table_header *)&data_end;
    Elf32_Sym * symbol_table = (char *)header + sizeof(struct symbol_table_header);
    char * str_table = (char *)header + sizeof(struct symbol_table_header) + header->symbol_table_size;
    uint32_t symnum = header->symbol_table_size / sizeof(Elf32_Sym);
    //print symbol
    print_symbol_table(symbol_table, str_table, 0x10, 0x20);
    //find and run symbol
    char * format = "LALALA %d %d\n";
    find_symbol_and_run(symbol_table, str_table, symnum, "printf", (uint32_t)format, 3, 4);
}