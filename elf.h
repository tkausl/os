#ifndef ELF_H
#define ELF_H
#include "types.h"

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

enum {
	ELF_ERR_NONE,
	ELF_ERR_WRONG_MAGIC,
	ELF_ERR_WRONG_CLASS,
	ELF_ERR_NO_PROGRAM_HEADER
};

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} elf_header_t;

typedef struct {
  uint32_t type;
  uint32_t offset;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t filesize;
  uint32_t memsize;
  uint32_t flags;
  uint32_t align;
} elf_ph_t;

#define	PF_R		0x4		/* p_flags */
#define	PF_W		0x2
#define	PF_X		0x1

/*
*	Loads the elf-file and their segments to whereever they need to be and returns the start-address
*/
uint32_t elf_load_executable(void* file);

uint32_t elf_last_error();

#endif