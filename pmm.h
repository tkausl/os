#ifndef PMM_H
#define PMM_H
#include "multiboot.h"
#include "types.h"

/*
*	Initializes the physical memory map
* First it frees any available memory,
* then it marks the whole kernel as used again
*/
void pmm_init(multiboot_info_t* mbt);

/*
*	Marks a physical page as used
*/
void pmm_set_used(uint32_t addr);

/*
*	Allocates a page of physical memory
*/
uint32_t pmm_first_free();

/*
*	Marks a physical page as free
*/
void pmm_free(uint32_t addr);

/*
*	Checks if a particular physical page is free
*/
bool is_free(uint32_t addr);


#define ADDR_TO_PAGE(var) (var/4096)
#define PAGE_TO_ADDR(var) (var*4096)

#endif