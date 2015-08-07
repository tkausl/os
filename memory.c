#include "memory.h"
#include "debug.h"
#include "vmm.h"

static heap_t kernel_heap;

void printf (const char *format, ...);

void init_kernel_heap() {
	extern const void KernelStart, KernelEnd;
	UNUSED(KernelStart);

	uint32_t KernelEndAddr = (uint32_t)&KernelEnd;

	kernel_heap.pos = kernel_heap.start = KernelEndAddr;
	kernel_heap.allocated_end = KernelEndAddr + MB(4); //Since the first 4 MB are mapped from the beginning we can just use that space
	kernel_heap.max_end = KernelEndAddr + MB(512);
}

void* kmalloc(uint32_t space, uint32_t alignment) {
	if(alignment > 1)
		kernel_heap.pos = ALIGN(kernel_heap.pos, alignment);

	while((kernel_heap.pos + space) >= kernel_heap.allocated_end) {
		alloc_frame(get_page(kernel_heap.allocated_end, false, kernel_directory), 0, 0);
		kernel_heap.allocated_end += 0x1000;
	}
	uint32_t tmp;
	tmp = kernel_heap.pos;
	kernel_heap.pos += space;
	return (void*)tmp;
}




void memset(void* ptr, uint8_t b, uint32_t size) {
	for(uint32_t i = 0; i < size; i++)
		*((uint8_t*)ptr+i) = b;
}

/*
* Simple Memcpy, copies char-by-char
* TODO implement better algorythm
* at the moment only fixes overlapping problem
*/

void memcpy(void* dest, void* src, uint32_t size) {
	if(dest == src) return;
	if(dest < src)
		while(size-- > 0)
			*(char*) dest++ = *(char*) src++;
	else {
		src += size;
		dest += size;
		while(size-- > 0)
			*(char*) --dest = *(char*) --src;
	}
}

void disable_pse() {
	asm volatile (	"mov %%cr4, %%eax\n"
					"and $~0x00000010, %%eax\n"
					"mov %%eax, %%cr4":::"eax");
}