#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/ports.h"
#include "printf.h"
#include "symbols.h"
#include "kernel.h"

extern int is_A20_on();

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



int sum_of_numbers(int a, int b) {
    printf("c = %d\n", a+b);
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

