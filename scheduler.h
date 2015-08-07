#ifndef PROCESS_H
#define PROCESS_H

#include "vmm.h"

typedef struct
{

/* 0 */   uint32_t ds;                  // Data segment selector

          /* pushed by pusha */
/* 4 */   uint32_t edi;
/* 8 */   uint32_t esi;
/* 12 */   uint32_t ebp;
/* 16 */   uint32_t esp;
/* 20 */   uint32_t ebx;
/* 24 */   uint32_t edx;
/* 28 */   uint32_t ecx;
/* 32 */   uint32_t eax;


		   /* pushed by the processor on interrupt*/
/* 36 */   uint32_t eip; //Instruction pointer
/* 40 */   uint32_t cs; //Code segment
/* 44 */   uint32_t eflags; // Flags

		   // Only pushed on ringwechsel!!!
/* 48 */   uint32_t useresp; // Stack ptr
/* 52 */   uint32_t ss; // Stack Segment
/* 56 */
} cpu_state_t;

enum {
	STATE_RUNNABLE,
	STATE_SLEEPING,
	STATE_DEAD
};


typedef struct process_struct {
	uint32_t process_id;
	uint32_t parent_process;
	uint32_t state;
	uint32_t sleeping;

	struct process_struct* next;

	page_directory_t* page_directory;
	uint32_t kernel_stack[1024];
	union {
		uint32_t* kernel_stack_ptr;
		cpu_state_t* cpu_state;
	};

} process_t;


void ks_push(process_t* process, uint32_t value);
uint32_t ks_pop(process_t* process);


process_t* processes;


process_t* new_process(page_directory_t* page_directory, uint32_t entry, uint32_t param, ...);

void process_fork();
#endif