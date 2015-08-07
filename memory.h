#include "types.h"
#include "multiboot.h"


#define KB(val) (val * 1024)
#define MB(val) (val * 1024 * 1024)
#define GB(val) (val * 1024 * 1024 * 1024)

#define PAGE_SIZE 4096

#define ALIGN(val, alignment) ((val + alignment - 1) & ~(alignment-1))
#define ALIGN_DOWN(val, alignment) (val & ~(alignment - 1))

#define IS_ALIGNED(val, alignment) ((val & (alignment-1)) == 0)

#define PAGE_ALIGNED(val) ALIGN(val, PAGE_SIZE)
#define IS_PAGE_ALIGNED(val) IS_ALIGNED(val, PAGE_SIZE)
#define IS_4B_ALIGNED(val) IS_ALIGNED(val, 4)



#define ZERO_MEM(mem) memset(mem, 0, sizeof(mem))

#define KERNEL_VIRTUAL_BASE 0xC0000000


void memset(void* ptr, uint8_t b, uint32_t size);
void memcpy(void* dest, void* src, uint32_t size);

typedef struct {
	uint32_t start;
	uint32_t allocated_end;
	uint32_t max_end;
	uint32_t pos;
} heap_t;


void init_kernel_heap();
/*
*	Allocate some space in kernel heap
*/
void* kmalloc(uint32_t space, uint32_t alignment);