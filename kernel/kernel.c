#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "printf.h"

extern int is_A20_on();
extern uint32_t end;
extern uint32_t code;
extern uint32_t data;
extern uint32_t data_end;
extern uint32_t bss;
extern uint32_t sector_num;

void A20_sanity_checks(){
    // Handle A20
    init_A20();
    unsigned char * addr = 0x011111;
    unsigned char * addr2 = 0x111111;
    *addr = 0x1;
    *addr2 = 0x2;
    if (*addr == *addr2 ) {
        printf("Equal addresses\n");
    }
    else {
        printf("Not Equal addresses\n");
    }

    if (is_A20_on()) {
        printf("A20_on\n");
    }
    else {
        printf("A20_off\n");
    }
}

void print_nice_hex(uint32_t * addr, int num) {
    uint32_t * start = addr;
    printf("Printing memory area starting at 0x%08x\n", start);
    uint32_t * real_start = start;
    for (int i=0; i < num/4; i++) {
        printf("memory at:0x%04x ", start);
        for (int j=0; j<4; j++) {
            if (i*4 + j >= num) {
                return;
            }
            printf("0x%08x ", *start);
//            if (*start == 0x00007c00 || *start == 0x007c0000) {
//                printf("\n Found Love at addr 0x%08x started at 0x%08x\n", start, real_start);
//                return;
//            }
            start += 1;
        }
        printf("\n");
    }
}

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

typedef void (*my_func)();


int sum_of_numbers(int a, int b) {
    printf("c = %d\n", a+b);
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

#define MAX_COMMAND_SIZE (256)
enum stage {
    COMMAND=0,
    PARAM1,
    PARAM2,
    PARAM3,
    END
};


static int isdigit (char c) {
    if ((c>='0') && (c<='9')) return 1;
    return 0;
}

static int skip_atoi(char *s)
{
    int i = 0;

    while (isdigit(*s)) {
        i = i * 10 + (*s - '0');
        s++;
    }
    return i;
}

uint32_t check_param(char * param) {
    int i = 0;
    bool is_str = false;
    uint32_t ret_val;
    while (param[i] == ' ') {
        i++;
    }
    if (param[i] == '"' ) {
        is_str = true;
        ret_val = (uint32_t)param;
    }
    if(!is_str) {
        ret_val = skip_atoi(param);
    }
    return ret_val;
}

void parse_command_and_run(char * input) {
    char command[MAX_COMMAND_SIZE] = {0};
    char param1[MAX_COMMAND_SIZE] = {0};
    char param2[MAX_COMMAND_SIZE] = {0};
    char param3[MAX_COMMAND_SIZE] = {0};
    int i = 0;
    int stage = COMMAND;
    int j = 0;

    struct symbol_table_header * header = (struct symbol_table_header *)&data_end;
    Elf32_Sym * symbol_table = (char *)header + sizeof(struct symbol_table_header);
    char * str_table = (char *)header + sizeof(struct symbol_table_header) + header->symbol_table_size;
    uint32_t symnum = header->symbol_table_size / sizeof(Elf32_Sym);

    while (input[i] != '\0' && stage != END) {
        printf("At stage %d\n", stage);
        switch (stage) {
            case COMMAND:
                while (input[i] == ' ') {
                    i++;
                }
                j = 0;
                while (input[i] != '(' && j < MAX_COMMAND_SIZE) {
                    command[j] = input[i];
                    i++;
                    j++;
                }
                i++;
                stage = PARAM1;
                break;
            case PARAM1:
                while (input[i] == ' ') {
                    i++;
                }
                j = 0;
                while (input[i] != ',' && j < MAX_COMMAND_SIZE) {
                    param1[j] = input[i];
                    i++;
                    j++;
                }
                i++;
                stage = PARAM2;
                break;
            case PARAM2:
                while (input[i] == ' ') {
                    i++;
                }
                j = 0;
                while (input[i] != ',' && j < MAX_COMMAND_SIZE) {
                    param2[j] = input[i];
                    i++;
                    j++;
                }
                i++;
                stage = PARAM3;
                break;
            case PARAM3:
                while (input[i] == ' ') {
                    i++;
                }
                j = 0;
                while (input[i] != ')' && j < MAX_COMMAND_SIZE) {
                    param3[j] = input[i];
                    i++;
                    j++;
                }
                i++;
                stage = END;
                break;
        }
    }

    uint32_t param1_val, param2_val, param3_val;
    param1_val = check_param(param1);
    param2_val = check_param(param2);
    param3_val = check_param(param3);
//    printf("Command %s, Param1 %s, Param2 %s, Param3 %s\n", command, param1, param2, param3);
//    printf("Command %s, Param1 %s, Param2 %d, Param3 %d\n", command, param1_val, param2_val, param3_val);
    find_symbol_and_run(symbol_table, str_table, symnum, command, param1_val, param2_val, param3_val);

}

void user_input(char *input) {
    if (strcmp(input, "halt") == 0) {
        printf("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    }
    else if(strcmp(input, "shutdown") == 0) {
        port_word_out(0x604, 0x2000);
    }
    else if(strcmp(input, "reboot") == 0) {
        reboot();
    }
    else if(strcmp(input, "cpuinfo") == 0) {
        unsigned int eax, edx;
        cpuid(1, &eax, &edx);
        printf("Cpuid is : eax %x  edx %x\n", eax ,edx);
        //apic:
        printf("apic supported %d\n", (edx & CPUID_FEAT_EDX_APIC) > 0);
        //MSR
        printf("msr supported %d\n", (edx & CPUID_FLAG_MSR ) > 0);
        if(edx & CPUID_FLAG_MSR) {
            cpuGetMSR(0x1B, &eax, &edx);
            printf("APIC BASE is 0x%8x\n", eax & 0xfffff000);
        }
    }
    else {
        printf("You said: %s \n>", input);
        parse_command_and_run(input);
    }
}



void main() {
    A20_sanity_checks();
    //Initializing all the processor interrupt related structures
    isr_install();
    IRQ_set_mask(CLOCK_INTERRUPT_LINE);
//    IRQ_set_mask(KEYBOARD_INTERRUPT_LINE);
    // Timer interrupt at 50HZ
    init_timer(50);
    // Keyboard interrupt
    init_keyboard();
    printf("The end of the kernel is at %08x\n", (uint32_t)&end);
    printf("The text of the kernel is at %08x\n", (uint32_t)&code);
    printf("The sector_num of the kernel is %d\n", (uint32_t)&sector_num);
    parse_symbol_table();
    initialise_paging();
    printf("Successful page table init\n");
//    int *ptr = (int*)0xA0000000;
//    int do_page_fault = *ptr;

}

void _start() {
    main();
}

