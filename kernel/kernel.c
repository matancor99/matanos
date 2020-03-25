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
    initialise_paging();
    printf("Successful page table init\n");
//    int *ptr = (int*)0xA0000000;
//    int do_page_fault = *ptr;

}

void _start() {
    main();
}

