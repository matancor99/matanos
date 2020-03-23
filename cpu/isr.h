#ifndef ISR_H
#define ISR_H

#include <stdint.h>

#define CPUID_FEAT_EDX_APIC         (1 << 9)
#define CPUID_FLAG_MSR         (1 << 5)



/* ISRs reserved for CPU exceptions */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* IRQ definitions */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#define IRQ0 32
#define CLOCK_INTERRUPT_LINE     (IRQ0 - IRQ0)
#define IRQ1 33
#define KEYBOARD_INTERRUPT_LINE     (IRQ1 - IRQ0)
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

static inline void io_wait(void)
{
    /* Port 0x80 is used for 'checkpoints' during POST. */
    /* The Linux kernel seems to think it is free for use :-/ */
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
    /* %%al instead of %0 makes no difference.  TODO: does the register need to be zeroed? */
}

static inline void disable_int(void) {
    asm volatile("cli");
}
static inline void enable_int(void) {
    asm volatile("sti");
}

static void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

/** issue a single request to CPUID. Fits 'intel features', for instance
 *  note that even if only "eax" and "edx" are of interest, other registers
 *  will be modified by the operation, so we need to tell the compiler about it.
 */
static inline void cpuid(int code, uint32_t *a, uint32_t *d) {
    asm volatile("cpuid":"=a"(*a),"=d"(*d):"a"(code):"ecx","ebx");
}

/* Struct which aggregates many registers */
typedef struct {
    unsigned int ds; /* Data segment selector */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; /* Pushed by pusha. */
    unsigned int int_no, err_code; /* Interrupt number and error code (if applicable) */
    unsigned int eip, cs, eflags, useresp, ss; /* Pushed by the processor automatically */
} registers_t;


#define __interrupt __attribute__((interrupt))
#define __packed __attribute__((packed))


__packed struct interrupt_frame {
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
};

__interrupt void isr3_handler(struct interrupt_frame *ir_frame);

void isr_install();
void isr_handler(registers_t r);
typedef void (*isr_t)(registers_t);
void register_interrupt_handler(unsigned char n, isr_t handler);
void IRQ_set_mask(unsigned char IRQline);
void IRQ_clear_mask(unsigned char IRQline);


#endif
