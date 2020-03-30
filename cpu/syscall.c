// syscall.c -- Defines the implementation of a system call system.
//              Written for JamesM's kernel development tutorials.

#include "syscall.h"
#include "isr.h"


static void syscall_handler(registers_t regs);


void print_string(char * str) {
    printf(str);
}

DEFN_SYSCALL1(print_string, 0, char*);

static void *syscalls[1] =
{
    &print_string,
};
uint32_t num_syscalls = 1;

void initialise_syscalls()
{
    // Register our syscall handler.
    register_interrupt_handler (0x80, syscall_handler);
}

void syscall_handler(registers_t regs)
{
    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.
    if (regs.eax >= num_syscalls)
        return;

    // Get the required syscall location.
    void *location = syscalls[regs.eax];

    // We don't know how many parameters the function wants, so we just
    // push them all onto the stack in the correct order. The function will
    // use all the parameters it wants, and we can pop them all back off afterwards.
    int ret;
    asm volatile (" \
      push %1; \
      push %2; \
      push %3; \
      push %4; \
      push %5; \
      call *%6; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
      pop %%ebx; \
    " : "=a" (ret) : "r" (regs.edi), "r" (regs.esi), "r" (regs.edx), "r" (regs.ecx), "r" (regs.ebx), "r" (location));
    regs.eax = ret;
}
