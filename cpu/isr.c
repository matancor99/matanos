#include "isr.h"
#include "idt.h"
#include "../drivers/screen.h"
#include "../kernel/util.h"

/* Can't do this with a loop because we need the address
 * of the function names */
void isr_install() {
    set_idt_gate(0, (unsigned int)isr0);
    set_idt_gate(1, (unsigned int)isr1);
    set_idt_gate(2, (unsigned int)isr2);
    set_idt_gate(3, (unsigned int)isr3_handler);
    set_idt_gate(4, (unsigned int)isr4);
    set_idt_gate(5, (unsigned int)isr5);
    set_idt_gate(6, (unsigned int)isr6);
    set_idt_gate(7, (unsigned int)isr7);
    set_idt_gate(8, (unsigned int)isr8);
    set_idt_gate(9, (unsigned int)isr9);
    set_idt_gate(10, (unsigned int)isr10);
    set_idt_gate(11, (unsigned int)isr11);
    set_idt_gate(12, (unsigned int)isr12);
    set_idt_gate(13, (unsigned int)isr13);
    set_idt_gate(14, (unsigned int)isr14);
    set_idt_gate(15, (unsigned int)isr15);
    set_idt_gate(16, (unsigned int)isr16);
    set_idt_gate(17, (unsigned int)isr17);
    set_idt_gate(18, (unsigned int)isr18);
    set_idt_gate(19, (unsigned int)isr19);
    set_idt_gate(20, (unsigned int)isr20);
    set_idt_gate(21, (unsigned int)isr21);
    set_idt_gate(22, (unsigned int)isr22);
    set_idt_gate(23, (unsigned int)isr23);
    set_idt_gate(24, (unsigned int)isr24);
    set_idt_gate(25, (unsigned int)isr25);
    set_idt_gate(26, (unsigned int)isr26);
    set_idt_gate(27, (unsigned int)isr27);
    set_idt_gate(28, (unsigned int)isr28);
    set_idt_gate(29, (unsigned int)isr29);
    set_idt_gate(30, (unsigned int)isr30);
    set_idt_gate(31, (unsigned int)isr31);

    set_idt(); // Load with ASM
}

/* To print the message which defines every exception */
char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t r) {
    kprint("received interrupt: ");
    char s[3];
    int_to_ascii(r.int_no, s);
    kprint(s);
    kprint("\n");
    kprint(exception_messages[r.int_no]);
    kprint("\n");
    if(r.int_no == 0) {
        r.eip += 2;
    }
}


__interrupt void isr3_handler(struct interrupt_frame *ir_frame)
{
    kprint("received interrupt 3\n");
}