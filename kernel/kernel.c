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

void parse_symbol_table() {
    struct symbol_table_header * header = (struct symbol_table_header *)&data_end;
    uint32_t * symbol_table = (char *)header + sizeof(struct symbol_table_header);
    uint32_t * str_table = (char *)header + sizeof(struct symbol_table_header) + header->symbol_table_size;
    print_nice_hex(symbol_table, 0x10);
    print_nice_hex(str_table, 0x10);
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

