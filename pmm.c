#include "pmm.h"
#include "memory.h"


/*
 * Der Einfachheit halber deklarieren wir die maximal benoetige Bitmapgroesse
 * statisch (Wir brauchen 4 GB / 4 kB = 1M Bits; 1M Bits sind 1M/32 = 32k
 * Eintraege fuer das Array)
 *
 * Willkuerliche Festlegung: 1 = Speicher frei, 0 = Speicher belegt
 */

#define ARR_OFFSET(var) (var/32)
#define BIT_OFFSET(var) (var%32)
#define FOR_EACH_BIT(varname) for(uint32_t varname = 0; varname < 32; ++varname)
#define BIT_TO_PAGE(arroff, bitoff) ((arroff) * 32 + (bitoff))
#define IS_BIT_SET(var, bit) ((var) & 1<<(bit))

#define UNSET_BIT(var, bit) var = var & ~(1<<bit)
#define SET_BIT(var, bit) var = var | (1<<bit);

#define ALL_USED(var) (var == 0x00000000)



#define BITMAP_SIZE 32768
static uint32_t bitmap[BITMAP_SIZE];
static uint32_t endFrame;

// For internal use. Set a chunk as free, faster than setting bit by bit
void pmm_set_free_chunk(uint32_t addr, uint32_t end);
void pmm_set_used_chunk(uint32_t addr, uint32_t end);

void pmm_init(multiboot_info_t* mbt){
	memset(bitmap, 0, sizeof(bitmap));
	endFrame = 0;

	multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbt->mmap_addr;
	while(mmap < (multiboot_memory_map_t*)(mbt->mmap_addr + mbt->mmap_length)) {
		if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE) { //Is this Memory available to us?
			uint32_t end = mmap->addr + mmap->len;
			pmm_set_free_chunk(ADDR_TO_PAGE(mmap->addr), ADDR_TO_PAGE(end));
			if(endFrame < ADDR_TO_PAGE(end))
				endFrame = ADDR_TO_PAGE(end);
		}
		mmap = (multiboot_memory_map_t*) ( (unsigned int)mmap + mmap->size + sizeof(unsigned int) );
	}

	//Define some global variables
	//extern const void PhysKernelStart, PhysKernelEnd;
	/*for(uint32_t i = ALIGN_DOWN((uint32_t)&PhysKernelStart, PAGE_SIZE); i < (uint32_t)PhysKernelEnd; i += PAGE_SIZE)
		pmm_set_used(i);*/
	//pmm_set_used_chunk(ADDR_TO_PAGE((uint32_t)&PhysKernelStart), ADDR_TO_PAGE(PAGE_ALIGNED((uint32_t)&PhysKernelEnd)));
	//pmm_set_used_chunk(ADDR_TO_PAGE((uint32_t)&PhysKernelStart), ADDR_TO_PAGE(PAGE_ALIGNED((uint32_t)&PhysKernelEnd)));

	pmm_set_used(0); //Set 0 as used as 0 is used as error
}

void pmm_set_free_chunk(uint32_t addr, uint32_t end) {
	while(addr < end && IS_ALIGNED(addr, 32)) //while the bit isn't aligned on a uint32_t boundary
		pmm_free(addr++); 				      // Set the page free

	while(addr < end - 31) {					//We are aligned on a uint32_t now
		bitmap[ARR_OFFSET(addr)] = 0xFFFFFFFF;  //Set the whole 32 bit free while there are more than 32 bits to set
		addr += 32;
	}

	while(addr < end)							//Set the last few bits free
		pmm_free(addr++);
}

void pmm_set_used_chunk(uint32_t addr, uint32_t end) {
	while(addr < end && IS_ALIGNED(addr, 32))
		pmm_set_used(addr++);

	while(addr < end - 31) {
		bitmap[ARR_OFFSET(addr)] = 0;
		addr += 32;
	}

	while(addr < end)
		pmm_set_used(addr++);
}

void pmm_set_used(uint32_t addr) {
	UNSET_BIT(bitmap[ARR_OFFSET(addr)], BIT_OFFSET(addr));
}

void pmm_free(uint32_t addr) {
	SET_BIT(bitmap[ARR_OFFSET(addr)], BIT_OFFSET(addr));
}

bool is_free(uint32_t addr) {
	return IS_BIT_SET(bitmap[ARR_OFFSET(addr)], BIT_OFFSET(addr));
}

uint32_t pmm_alloc_first_free() {
	uint32_t page = pmm_first_free();
	pmm_set_used(page);
	return page;
}

uint32_t pmm_first_free() {
	for(uint32_t i = 0; i < ARR_OFFSET(endFrame); ++i) {
		if(!ALL_USED(bitmap[i])) {
			FOR_EACH_BIT(j) {
                if ( IS_BIT_SET(bitmap[i], j) )
                {
                    return BIT_TO_PAGE(i, j);
                }
           }
		}
	}
	return 0;
}
