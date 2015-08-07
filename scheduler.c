#include "scheduler.h"
#include "vmm.h"
#include "debug.h"
#include "gdt.h"

process_t* current = 0;

process_t* processes = 0;

process_t* sleep_forever_process;


uint32_t next_pid = 1;

cpu_state_t* schedule();

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
    /* TODO: Should %1 be %w1? */
}

cpu_state_t* timer_int(cpu_state_t* state) {
	//Ticking...
	process_t* tmp = processes;
	while(tmp) {
		if(tmp->state == STATE_SLEEPING) {
			tmp->sleeping--;
			if(tmp->sleeping == 0) {
				tmp->state = STATE_RUNNABLE;
			}
		}
		tmp = tmp->next;
	}

	cpu_state_t* newstate = schedule(state);
	if(state != newstate) {
		switch_page_directory(current->page_directory);
		tss_table.esp0 = (uint32_t)&current->kernel_stack[1024];
	}
	outb(0x20, 0x20);
	return newstate;
}

cpu_state_t* process_yield(cpu_state_t* state) {
	cpu_state_t* newstate = schedule(state);
	if(state != newstate) {
		switch_page_directory(current->page_directory);
		tss_table.esp0 = (uint32_t)&current->kernel_stack[1024];
	}
	return newstate;
}


uint32_t current_task_id = 0;

cpu_state_t* schedule(cpu_state_t* state) {
	if(current)
		current->cpu_state = state;

	process_t* start;
	process_t* next;
	if(!processes) {
		next = sleep_forever_process;
	} else {
		if(current == sleep_forever_process || current == 0) {
			process_t* tmp = processes;
			while(tmp->next)
				tmp = tmp->next;
			start = tmp;
			next = processes;
		} else {
			start = current;
			next = current->next;
		}
		while(true) {
			if(!next) {
				next = processes;
			}

			if(!next) {
				next = sleep_forever_process;
				break;
			}

			if(next->state == STATE_RUNNABLE) {
				break;
			}

			if(next == start) { //We're back at the beginning, what now?
				next = sleep_forever_process;
				break;
			}
			next = next->next;
		}
	}
	current = next;
	return current->cpu_state;
}

void ks_push(process_t* process, uint32_t value) {
	*(--process->kernel_stack_ptr) = value;
}

uint32_t ks_pop(process_t* process) {
	return *(process->kernel_stack_ptr++);
}


typedef __builtin_va_list va_list;
#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)
#define va_arg(v,l)	__builtin_va_arg(v,l)

process_t* new_process(page_directory_t* page_directory, uint32_t entry, uint32_t params, ...) {
  process_t* process = (process_t*)kmalloc(sizeof(process_t), 4);

  process->process_id = next_pid++;

  process->kernel_stack_ptr = (uint32_t*)&process->kernel_stack[1024];
  process->page_directory = page_directory;
  process->state = STATE_RUNNABLE;
  process->next = 0;
  
  va_list args;
  va_start(args, params);

  uint32_t eflags;
  asm volatile("pushf; pop %0" : "=r"(eflags));


  ks_push(process, 0x20 | 0x03);
  ks_push(process, 0xC0000000);

  ks_push(process, 0x202);//ks_push(process, eflags); //Eflags
  ks_push(process, 0x18 | 0x03); //Code Segment

  ks_push(process, (uint32_t)entry);

  uint32_t tmp = (uint32_t)process->kernel_stack_ptr;

  ks_push(process, 0); //EAX - EDX
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0);

  ks_push(process, tmp);
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0x20 | 0x03);
  return process;
}

void sleep_forever_entry() {
	printf("Sleep-Forever started sleeping...\n");
	while(true) {
		asm volatile ("hlt");
	}
	printf("Sleep-Forever STOPPED sleeping...\n");
}

void init_sleep_forever() {
  process_t* process = (process_t*)kmalloc(sizeof(process_t), 4);

  process->process_id = 0;

  process->kernel_stack_ptr = (uint32_t*)&process->kernel_stack[1024];
  process->page_directory = kernel_directory;
  process->state = STATE_RUNNABLE;
  process->next = 0;

  ks_push(process, 0x202);//ks_push(process, eflags); //Eflags
  ks_push(process, 0x08); //Code Segment

  ks_push(process, (uint32_t)sleep_forever_entry);

  uint32_t tmp = (uint32_t)process->kernel_stack_ptr;

  ks_push(process, 0); //EAX - EDX
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0);

  ks_push(process, tmp);
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0);
  ks_push(process, 0x10);
  sleep_forever_process = process;
}

void process_sleep(cpu_state_t* regs) {
	current->state = STATE_SLEEPING;
	current->sleeping = regs->ecx;
	asm volatile ("int $0x81");
}

void process_fork(cpu_state_t* regs) {
	printf("Process %d requested a fork...\n", current->process_id);
	process_t* first = current;
	process_t* second = (process_t*)kmalloc(sizeof(process_t), 4);
	*second = *first;
	uint32_t stack_len = ((uint32_t)&first->kernel_stack[1024]) - ((uint32_t)regs);
	second->kernel_stack_ptr = ((uint32_t)&second->kernel_stack[1024]) - stack_len;
	second->cpu_state->ebx = 0;
	second->process_id = next_pid++;
	second->parent_process = first->process_id;
	second->page_directory = copy_directory(first->page_directory);
	regs->ebx = second->process_id;


	second->next = processes;
	processes = second;
}

void process_exit(cpu_state_t* regs) {
	printf("Process %d (parent %d) exited with exit-status %d\n", current->process_id, current->parent_process, regs->ecx);

	current->state = STATE_DEAD;
	asm volatile ("int $0x81"); //Reschedule
}