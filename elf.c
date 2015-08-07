#include "elf.h"
#include "vmm.h"

static uint32_t last_error;

#ifndef KERNEL_VIRTUAL_BASE
#define KERNEL_VIRTUAL_BASE 0xC0000000
#endif

uint32_t elf_load_executable(void* file) {
	elf_header_t* header = (elf_header_t*)file;

	if(header->e_ident[0] != ELFMAG0 || header->e_ident[1] != ELFMAG1 || header->e_ident[2] != ELFMAG2 || header->e_ident[3] != ELFMAG3) {
		last_error = ELF_ERR_WRONG_MAGIC;
		return 0;
	}

	if(header->e_ident[4] != ELFCLASS32) {
		last_error = ELF_ERR_WRONG_CLASS;
		return 0;
	}

	if(header->e_phoff == 0 || header->e_phnum == 0) {
		last_error = ELF_ERR_NO_PROGRAM_HEADER;
		return 0;
	}

	for(uint32_t i = 0; i < header->e_phnum; i++) {
		elf_ph_t* program_header = (elf_ph_t*)(((uint32_t)file) + header->e_phoff + (header->e_phentsize * i));
		printf("Found ProgramHeader type 0x%x\n", program_header->type);
		if(program_header->type == 1) { //Type 'LOAD'
			printf("Loading Header from 0x%x size 0x%x to 0x%x size 0x%x\n", program_header->offset, program_header->filesize, program_header->vaddr, program_header->memsize);
			vmm_map_range(program_header->vaddr, program_header->vaddr + program_header->memsize, 0, ((program_header->flags & PF_W) == PF_W));
			memcpy((void*)program_header->vaddr, (void*)(((uint32_t)file) + program_header->offset), program_header->memsize);
		}
	}

	vmm_map_range(KERNEL_VIRTUAL_BASE - 2 * 4096, KERNEL_VIRTUAL_BASE, 0, 1);

	return header->e_entry;
}

uint32_t elf_last_error() {
	return last_error;
}