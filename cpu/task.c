// 
// task.c - Implements the functionality needed to multitask.
//          Written for JamesM's kernel development tutorials.
//

#include "task.h"
#include "paging.h"
#include "kheap.h"
#include "isr.h"
#include "kernel.h"
#include "util.h"

// The currently running task.
volatile task_t *current_task;
volatile task_t * tmp_current_task = NULL;

// The start of the task linked list.
volatile task_t *ready_queue;

volatile task_t *sleep_queue = NULL;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern uint32_t read_eip();

// The next available process ID.
uint32_t next_pid = 1;

void initialise_tasking()
{
    // Rather important stuff happening, no interrupts please!
    asm volatile("cli");

    // Relocate the stack so we know where it is. The moving of the stuck is not as important
    // As allocating the table as if it is not in the kernel directory (notice that we are adding the
    // Table to the current directory.
    // The importance is that when we make a clone of the table we will not refer when accessing the stack to
    // The same directory (assentially be calling clone_table over this table in clone_directory.
    move_stack((void*)0xE0000000, KERNEL_STACK_SIZE);

    // Initialise the first task (kernel task)
    current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
    current_task->id = next_pid++;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = current_directory;
    current_task->next = 0;
    current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    current_task->end_of_sleep = 0;
    // Reenable interrupts.
    asm volatile("sti");
}

void move_stack(void *new_stack_start, uint32_t size)
{
  uint32_t i;
  uint32_t initial_esp = (uint32_t)&end + KERNEL_STACK_SIZE;
  // Allocate some space for the new stack.
  for( i = (uint32_t)new_stack_start;
       i >= ((uint32_t)new_stack_start-size);
       i -= 0x1000)
  {
    // General-purpose stack is in user-mode.
    alloc_frame( get_page_ptr_in_page_table(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
  }
  
  // Flush the TLB by reading and writing the page directory address again.
  uint32_t pd_addr;
  asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
  asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

  // Old ESP and EBP, read from registers.
  uint32_t old_stack_pointer; asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  uint32_t old_base_pointer;  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

  // Offset to add to old stack addresses to get a new stack address.
  uint32_t offset            = (uint32_t)new_stack_start - initial_esp;

  // New ESP and EBP.
  uint32_t new_stack_pointer = old_stack_pointer + offset;
  uint32_t new_base_pointer  = old_base_pointer  + offset;

  // Copy the stack.
  memcpy((void*)old_stack_pointer, (void*)new_stack_pointer, initial_esp-old_stack_pointer);

  // Backtrace through the original stack, copying new values into
  // the new stack.
  for(i = (uint32_t)new_stack_start; i > (uint32_t)new_stack_start-size; i -= 4)
  {
    uint32_t tmp = * (uint32_t*)i;
    // If the value of tmp is inside the range of the old stack, assume it is a base pointer
    // and remap it. This will unfortunately remap ANY value in this range, whether they are
    // base pointers or not.
    if (( old_stack_pointer < tmp) && (tmp < initial_esp))
    {
      tmp = tmp + offset;
      uint32_t *tmp2 = (uint32_t*)i;
      *tmp2 = tmp;
    }
  }

  // Change stacks.
  asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

void switch_task()
{
    // If we haven't initialised tasking yet, just return.
    int task_counter = 1;
    if (!current_task)
        return;

    // If there is only one task - return
    if (current_task == ready_queue && ready_queue->next == NULL) {
        return;
    }

    // Read esp, ebp now for saving later on.
    uint32_t esp, ebp, eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));

    // Read the instruction pointer. We do some cunning logic here:
    // One of two things could have happened when this function exits - 
    //   (a) We called the function and it returned the EIP as requested.
    //   (b) We have just switched tasks, and because the saved EIP is essentially
    //       the instruction after read_eip(), it will seem as if read_eip has just
    //       returned.
    // In the second case we need to return immediately. To detect it we put a dummy
    // value in EAX further down at the end of this function. As C returns values in EAX,
    // it will look like the return value is this dummy value! (0x12345).
    eip = read_eip();
//    printf("eip is 0x%08x, cur task id is %d, next task is 0x%08x\n", eip, current_task->id, current_task->next);
    // Have we just switched tasks?
    if (eip == 0x12345)
        return;

    // No, we didn't switch tasks. Let's save some register values and switch.
    current_task->eip = eip;
    current_task->esp = esp;
    current_task->ebp = ebp;
    
    // Get the next task to run.
    tmp_current_task = current_task->next;
    // If we fell off the end of the linked list start again at the beginning.
    if (!tmp_current_task) tmp_current_task = ready_queue;

    // Remove current task from ready queue if is in sleep state
    if (current_task->end_of_sleep) {
        task_t *tmp_task = (task_t *) ready_queue;
        //If current task starts the list
        if (ready_queue == current_task) {
            ready_queue = current_task->next;
        }
        else {
            //Find the pointer in the list that points at current task
            while (tmp_task->next != current_task) {
                tmp_task = tmp_task->next;
            }
            // Skip current task
            tmp_task->next = current_task->next;
        }
        //Reset the current task next pointer
        current_task->next = NULL;
    }

    current_task = tmp_current_task;

    eip = current_task->eip;
    esp = current_task->esp;
    ebp = current_task->ebp;

    // Make sure the memory manager knows we've changed page directory.
    current_directory = current_task->page_directory;

    // Change our kernel stack over.
    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);
    // Here we:
    // * Stop interrupts so we don't get interrupted.
    // * Temporarily puts the new EIP location in ECX.
    // * Loads the stack and base pointers from the new task struct.
    // * Changes page directory to the physical address (physicalAddr) of the new directory.
    // * Puts a dummy value (0x12345) in EAX so that above we can recognise that we've just
    //   switched task.
    // * Restarts interrupts. The STI instruction has a delay - it doesn't take effect until after
    //   the next instruction.
    // * Jumps to the location in ECX (remember we put the new EIP in there).
    asm volatile("         \
      mov %0, %%ecx;       \
      mov %1, %%esp;       \
      mov %2, %%ebp;       \
      mov %3, %%cr3;       \
      mov $0x12345, %%eax; \
      jmp *%%ecx           "
                 : : "r"(eip), "r"(esp), "r"(ebp), "r"(current_directory->physicalAddr));
}

int fork()
{
    // We are modifying kernel structures, and so cannot
    asm volatile("cli");

    // Take a pointer to this process' task struct for later reference.
    task_t *parent_task = (task_t*)current_task;

    // Clone the address space.
    page_directory_t *directory = clone_directory(current_directory);

    // Create a new process.
    task_t *new_task = (task_t*)kmalloc(sizeof(task_t));

    new_task->id = next_pid++;
    new_task->esp = new_task->ebp = 0;
    new_task->eip = 0;
    new_task->page_directory = directory;
    new_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
    new_task->next = 0;
    new_task->end_of_sleep = 0;

    // Add it to the end of the ready queue.
    task_t *tmp_task = (task_t*)ready_queue;
    while (tmp_task->next)
        tmp_task = tmp_task->next;
    tmp_task->next = new_task;

    // This will be the entry point for the new process.
    uint32_t eip = read_eip();


    // We could be the parent or the child here - check.
    if (current_task == parent_task)
    {
        // We are the parent, so set up the esp/ebp/eip for our child.
        uint32_t esp; asm volatile("mov %%esp, %0" : "=r"(esp));
        uint32_t ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
        new_task->esp = esp;
        new_task->ebp = ebp;
        new_task->eip = eip;
        asm volatile("sti");

        return new_task->id;
    }
    else
    {
        // We return from interrupt context - after the first time context switch happens to here.
        // We dont really care that we fuck the registers here because it is the end of the function and we not longer use them so, pasten:
        // This is very bad programming but when looking at the assembly flow from  uint32_t eip = read_eip(); to the return 0;
        // no register value is used seriously, thus this method is safe but very not portable for any changes in the flow of fork.
        // Moreover, we really dont care that we jump here from interrupt context because we recover the esp.
        asm volatile("sti");
        // We are the child.   No need to call sti because we get here through context switch.
        return 0;
    }

}

int getpid()
{
    return current_task->id;
}


void switch_to_user_mode()
{
    // Set up our kernel stack.
    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);

    // Set up a stack structure for switching to user mode.
    /* This logic:
     * pop %eax;
      or $0x200, %eax;
      push %eax; \
     * Meant to alter the eflags to enable interrupts, sti not working
     * */
    asm volatile("  \
      cli; \
      mov $0x23, %ax; \
      mov %ax, %ds; \
      mov %ax, %es; \
      mov %ax, %fs; \
      mov %ax, %gs; \
                    \
       \
      mov %esp, %eax; \
      pushl $0x23; \
      pushl %esp; \
      pushf; \
      pop %eax;  \
      or $0x200, %eax; \
      push %eax; \
      pushl $0x1B; \
      push $1f; \
      iret; \
    1: \
      add $0x4, %esp\
      ");
    // Im not sure why but i was missing 4 bytes for the esp to return to main in the end of the inline assembly
}
