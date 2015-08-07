#include "vmm.h"
#include "pmm.h"
#include "debug.h"
#include "memory.h"
#include "terminal.h"

#define LABEL(label) ({extern const void label; (uint32_t)&label;})


page_directory_t* current_directory;
page_directory_t* kernel_directory;


/*

int __builtin_popcount (unsigned int x);

uint32_t NumberOfSetBits(uint32_t i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}*/
#define ARR_OFFSET(var) (var/32)
#define BIT_OFFSET(var) (var%32)
#define FOR_EACH_BIT(varname) for(uint32_t varname = 0; varname < 32; ++varname)
#define BIT_TO_PAGE(arroff, bitoff) ((arroff) * 32 + (bitoff))
#define IS_BIT_SET(var, bit) ((var) & 1<<(bit))

#define UNSET_BIT(var, bit) var = var & ~(1<<bit)
#define SET_BIT(var, bit) var = var | (1<<bit);

#define ALL_USED(var) (var == 0x00000000)

uint32_t temp_bitset[32]; // 1024 bits for page 1023 (id 1022) for temporary mapped pages
/*
	1024 = 1<<10
	4096 = 1<<12
	One page = 4096 bytes or 0x1000
	One page table = 1024 entries of 4096 bytes = 4194304 bytes or 0x40 0000
	Temp-mapped memory at 0xFF80 0000 to 0xFFC0 0000
*/

uint32_t temp_map(uint32_t physical) {
	for(uint32_t i = 0; i < 32; ++i) {
		if(!ALL_USED(temp_bitset[i])) {
			FOR_EACH_BIT(j) {
                if ( IS_BIT_SET(temp_bitset[i], j) )
                {
                	map_frame(get_page((BIT_TO_PAGE(i, j)<<12) + 0xFF800000, 0, kernel_directory), physical>>12, 0, 0);
                	UNSET_BIT(temp_bitset[i], j);
                    return (BIT_TO_PAGE(i, j)<<12) + 0xFF800000;
                }
           }
		}
	}
	return 0;
}

uint32_t temp_unmap(uint32_t virtual) {
	unmap_frame(get_page(virtual, 0, kernel_directory));
	asm volatile ("invlpg (%0)" ::"r"(virtual) : "memory");
}

uint32_t find_physical_addr(page_directory_t *dir, uint32_t virtual) {
	uint32_t id1 = virtual >> 22;
	uint32_t id2 = (virtual >> 12) & 0x3FF;
	return ((dir->tables[id1]->pages[id2].frame) << 12) + (virtual & 0xFFF);
}

page_t *get_page_boot(uint32_t address, int make, page_directory_t *dir);

void initialize_paging() {
	memset(temp_bitset, 0xFF, sizeof(temp_bitset));

	kernel_directory = (page_directory_t*)LABEL(kernel_page_directory);
	memset(kernel_directory, 0, sizeof(*kernel_directory));
	kernel_directory->physicalAddr = ((uint32_t)&kernel_directory->tablesPhysical) - KERNEL_VIRTUAL_BASE;


	uint32_t kernel_pages = LABEL(kernel_page_tables); //Enough room for 256 pages
	memset((void*)kernel_pages, 0, 256 * PAGE_SIZE);

	for(uint32_t i = 0; i < 255; ++i) { // 768 to 1022 mapped identity, last one mapped to itself
		kernel_directory->tables[i + 768] = (page_table_t*)(kernel_pages + i * sizeof(page_table_t));
		kernel_directory->tablesPhysical[i + 768] = ((kernel_pages + i * sizeof(page_table_t)) - KERNEL_VIRTUAL_BASE) | 0x7;
	}

	kernel_directory->tables[1023] = (page_table_t*)(kernel_directory);
	kernel_directory->tablesPhysical[1023] = kernel_directory->physicalAddr | 0x7;


	uint32_t KernelStartAddr = LABEL(KernelStart);
	printf("Kernel Start is at 0x%x\n", KernelStartAddr);
	for(uint32_t i = KERNEL_VIRTUAL_BASE; i < (LABEL(KernelEnd) + MB(4)); i += 0x1000) {
		map_frame(get_page(i, false, kernel_directory), (i-KERNEL_VIRTUAL_BASE)/4096, 0, 0);
		pmm_set_used((i-KERNEL_VIRTUAL_BASE)/4096);
	}
	printf("Directory struct is at V0x%x\n", kernel_directory);
	printf("Physical is at: V0x%x\n", &kernel_directory->tablesPhysical);
	switch_page_directory(kernel_directory);
}

void switch_page_directory(page_directory_t *dir) {
	current_directory = dir;
	asm volatile ("mov %0, %%cr3" :: "r"(dir->physicalAddr));
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir) {
	address /= 0x1000;
	uint32_t table_id = address / 1024;
	//uint32_t table_index = address%1024;

	if (dir->tables[table_id])
		return &dir->tables[table_id]->pages[address%1024];

	if(!make) {
		printf("Needed a new page-table but make was false\n");
		return 0;
	}

	printf("FrameTable not available. Allocating new one...\n");

	uint32_t frame = pmm_first_free();
	pmm_set_used(frame);
	frame <<= 12;
	dir->tablesPhysical[table_id] = frame | 0x7;
	dir->tables[table_id] = (page_table_t*)(((uint32_t)0xFFC00000) + (0x1000 * table_id));
	memset(dir->tables[table_id], 0, 0x1000);

	return &dir->tables[table_id]->pages[address%1024];
}

void alloc_frame(page_t *page, int is_kernel, int is_writeable) {
	if(page->frame)
		return; //we already have a frame

	uint32_t frame_id = pmm_first_free();
	ASSERT(frame_id);
	pmm_set_used(frame_id);
	map_frame(page, frame_id, is_kernel, is_writeable);
}

void map_frame(page_t *page, uint32_t frame_id, int is_kernel, int is_writeable) {
	page->present = 1;
	page->rw = (is_writeable == 1);
	page->user = (is_kernel == 0);
	page->frame = frame_id;
}

void free_frame(page_t *page) {
	if(!page->frame)
		return; //we have no frame
	pmm_free(page->frame);
	unmap_frame(page);
}

void unmap_frame(page_t *page) {
	page->frame = 0;
	page->present = 0;
}

typedef struct {
union {
	struct {
	uint32_t present : 1; // 1
	uint32_t write   : 1; // 2
	uint32_t user    : 1; // 3
	uint32_t ignore  : 9; // 12
	uint32_t frame    : 20; // 32
};
uint32_t whole;
};
} page_directory_entry;

typedef struct {
	union {
	struct {
	uint32_t present : 1; // 1
	uint32_t write   : 1; // 2
	uint32_t user    : 1; // 3
	uint32_t ignore  : 9; // 12
	uint32_t frame    : 20; // 32
	};
uint32_t whole;
};
} page_table_entry;

typedef struct {
	union {
		struct {
			uint32_t present		: 1;  // 1
			uint32_t write			: 1;  // 2
			uint32_t user			: 1;  // 3
			uint32_t writethrough	: 1;  // 4
			uint32_t cache_disabled	: 1;  // 5
			uint32_t accessed		: 1;  // 6
			uint32_t dirty			: 1;  // 7
			uint32_t ignore0		: 1;  // 8
			uint32_t global			: 1;  // 9
			uint32_t avail			: 3;  // 12
			uint32_t frame 			: 20; // 32
		};
		uint32_t val;
	};
} test_page_table_entry;

typedef struct {
	union {
		struct {
			uint32_t present		: 1;  // 1
			uint32_t write			: 1;  // 2
			uint32_t user			: 1;  // 3
			uint32_t writethrough	: 1;  // 4
			uint32_t cache_disabled	: 1;  // 5
			uint32_t accessed		: 1;  // 6
			uint32_t ignore0		: 1;  // 7
			uint32_t big_pages		: 1;  // 8
			uint32_t ignore1		: 1;  // 9
			uint32_t avail			: 3;  // 12
			uint32_t frame 			: 20; // 32
		};
		uint32_t val;
	};
} test_page_directory_entry;

typedef struct {
	test_page_table_entry page[1024];
} test_page_table;

typedef struct {
	union {
		test_page_table table[1023];
		test_page_table_entry [1024 * 1023];
	};
	test_page_directory_entry directory[1023];
	uint32_t ignore;
} test_page_directory;

test_page_directory* global_dir = (test_page_directory*)0xFFC00000;


page_directory_entry* get_page_directory() {
	return (page_directory_entry*)0xFFFFF000;
}

page_table_entry* get_page_table(uint32_t id) {
	return (page_table_entry*)(0xFFC00000 + 0x1000 * id);
}

uint32_t physical_address(uint32_t virtual) {
	uint32_t pdindex = (uint32_t)virtual >> 22;
    uint32_t ptindex = (uint32_t)virtual >> 12 & 0x03FF;
    /*
	page_directory_entry* pd = get_page_directory();
	page_table_entry* pt = get_page_table(pdindex);
	if(!pd[pdindex].frame)
		return 0;
	if(!pt[ptindex].frame)
		return 0;
	return pt[ptindex].frame << 12;*/

	if(!global_dir->directory[pdindex].frame)
		return 0;
	if(!global_dir->table[pdindex].page[ptindex].frame)
		return 0;
	return global_dir->table[pdindex].page[ptindex].frame << 12;
}

page_directory_t* copy_directory() {
  page_directory_t* dir = kmalloc(sizeof(page_directory_t), PAGE_SIZE);
  dir->physicalAddr = physical_address((uint32_t)dir->tablesPhysical);
  dir->tables[1023] = (page_table_t*)(dir);
  dir->tablesPhysical[1023] = dir->physicalAddr | 0x7;

  page_directory_entry* origi = get_page_directory();

  for(uint32_t i = 768; i < 1023; i++) { //Kernel pages
  	dir->tablesPhysical[i] = origi[i].whole;
  }
  uint32_t num = 0;
  uint32_t numt = 0;
  for(uint32_t i = 0; i < 768; i++) {
  	if(origi[i].frame) {
  		numt++;
  		page_table_entry* origit = get_page_table(i);

  		uint32_t table_frame = pmm_alloc_first_free();
  		uint32_t* table_ptr = (uint32_t*)temp_map(table_frame * 4096);
  		memset(table_ptr, 0, 4096);

  		for(uint32_t j = 0; j < 1024; j++) {
  			if(origit[j].frame) {
  				uint32_t data_frame = pmm_alloc_first_free();
  				uint32_t data_ptr = temp_map(data_frame * 4096);
  				uint32_t virtual = (i << 22) | (j << 12);
  				memcpy((void*)data_ptr, (void*)virtual, 4096);
  				temp_unmap(data_ptr);
  				table_ptr[j] = (data_frame * 4096) | 0x7;
  				num++;
  			}
  		}
  		temp_unmap(table_ptr);
	  	//dir->tables[i] = orig->tables[i];
	  	dir->tablesPhysical[i] = (table_frame * 4096) | 0x7;
    }
  }
  printf("Copied %d pages over from %d tables\n", num, numt);
  return dir;
}


page_directory_t* new_directory() {
  page_directory_t* dir = kmalloc(sizeof(page_directory_t), PAGE_SIZE);
  memcpy(dir, kernel_directory, sizeof(page_directory_t));

  dir->physicalAddr = find_physical_addr(dir, (uint32_t)dir->tablesPhysical);

  dir->tables[1023] = (page_table_t*)(dir);
  dir->tablesPhysical[1023] = dir->physicalAddr | 0x7;
  return dir;
}


void vmm_map_range(uint32_t begin, uint32_t end, bool kernel, bool write) {
	begin = ALIGN_DOWN(begin, PAGE_SIZE);
	end = ALIGN(end, PAGE_SIZE);

	while(begin < end) {
		alloc_frame(get_page(begin, 1, current_directory), kernel, write);
		begin += PAGE_SIZE;
	}
}