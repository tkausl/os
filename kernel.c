#include "types.h"

#include "debug.h"


static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
    /* TODO: Should %1 be %w1? */
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
    /* TODO: Should %1 be %w1? */
    return ret;
}

#include "multiboot.h"
#include "memory.h"
#include "gdt.c"
#include "pmm.h"
#include "vmm.h"
#include "terminal.h"
#include "scheduler.h"
#include "elf.h"




typedef struct registers
{
   u32int ds;                  // Data segment selector
   u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   u32int int_no, err_code;    // Interrupt number and error code (if applicable)
   u32int eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;


extern void jump_to_user();




void kernel_main(uint32_t mbt_physical_address, unsigned int magic) {
	UNUSED(magic);
  /*
  * State when this function is called:
  * We have a unknown section table set up by grub. We have a 16K stack set up by our boot assembler code
  * We have paging set up, 4MB maps enabled, a single 4MB page maps 3GB to 0 (exactly 4MB space to use.) 
  * Our Kernel should reside at exactly 3GB + 1MB (Physical: 1MB)
  *
  * What we have to do now is, take all the data out of the GRUB-structures that we need but while we do this we *CAN'T* allocate space for them,
  * so we need to use fixed variables and arrays, which will, of course, reside in our kernels data space
  */
  terminal_initialize();

  // Fix various pointers
  multiboot_info_t* mbt = (multiboot_info_t*)(mbt_physical_address + KERNEL_VIRTUAL_BASE);
  mbt->mods_addr += KERNEL_VIRTUAL_BASE;
  mbt->mmap_addr += KERNEL_VIRTUAL_BASE;
  ASSERT(mbt->mods_count == 1); //Shouldn't happen


  // They both don't allocate mem
  init_gdt();
  init_idt();


  //Init memory map
  pmm_init(mbt);
  initialize_paging();
  init_kernel_heap();


  multiboot_module_t* modlist = (multiboot_module_t*)mbt->mods_addr;

  void* initrd = kmalloc(modlist->mod_end - modlist->mod_start, 4);
  memcpy(initrd, (void*)(modlist->mod_start + KERNEL_VIRTUAL_BASE), modlist->mod_end - modlist->mod_start);




  page_directory_t* p1d = new_directory();
  switch_page_directory(p1d);

  uint32_t initrd_entry = elf_load_executable(initrd);
  printf("Initrd Entry is at 0x%x\n", initrd_entry);

  init_sleep_forever();
  
  process_t* p1 = new_process(p1d, initrd_entry, 0);

  processes = p1;


  uint32_t divisor = 1193180 / 100;
  outb(0x43, 0x36); //Enable with 0x36

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  u8int l = (u8int)(divisor & 0xFF);
  u8int h = (u8int)( (divisor>>8) & 0xFF );

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);

  /*
        outb(0x21,0xfd);
   outb(0xa1,0xff);*/

  BOCHS_BREAK;
  asm volatile ("int $0x81");

  //switch_page_directory(initrd_directory);





	printf("Hello, kernel World!\nThis is a little test!\n");

	update_cursor(10, 10);



  BOCHS_BREAK;
  asm volatile ("int $0x20");


  asm volatile ("int $0x80");
  jump_to_user();
	asm volatile ("int $0x80");
  printf("First Interrupt end!!\n");
	asm volatile ("int $0x80");
  printf("Second Interrupt end!!\n");

for(;;);




   /*
      outb(0x21,0xfd);
   outb(0xa1,0xff);
   */

}

void user_func() {
  printf("Hello from User-Mode");
}



void isr_handler(registers_t* regs)
{
  switch(regs->int_no) {
    case 0xE: {//Page Fault
      uint32_t pfaddr;
      asm volatile ("mov %%cr2, %0" : "=r"(pfaddr));
      printf("PageFault trying to access address 0x%x", pfaddr);
      if(regs->err_code & 0x1) {
        printf(" (page-protection violation)");
      } else {
        printf(" (page not present)");
      }
      if(regs->err_code & 0x2) {
        printf(" (write)");
      } else {
        printf(" (read)");
      }

      if(regs->err_code & 0x4) {
        printf(" (USER)");
      } else {
        printf(" (KERNEL)");
      }

      if(regs->err_code & 0x10) {
        printf(" (instruction-fetch)");
      } else {
        printf(" (no instruction-fetch)");
      }
      printf("\n"); }
    break;
    default:
      printf("Received Interrupt: %x\n", regs->int_no);    
  }
  if(regs->cs == 0x08) {
    printf("PANIC! THIS INTERRUPT CAME FROM KERNEL-CODE\n", regs->int_no);
    asm volatile ("jt: cli\nhlt\n jmp jt");
  }
}

void irq_handler(registers_t* regs){
   printf("Got HARDWARE-Interrupt %x\n", regs->int_no);
   if(regs->int_no == 1) {
      //keyboard!!
      uint8_t b = inb(0x60);
      uint8_t b2 = inb(0x60);
      printf("Got Scancode %x, %x\n", b, b2);
   }

  if (regs->int_no >= 8)
   {
       outb(0xA0, 0x20);
   }
   outb(0x20, 0x20);
}


extern process_t* current;
void usr_int(cpu_state_t* regs) {
  switch(regs->eax) {
    case 0: //Exit
      process_exit(regs);
      break;
    case 1: //fork
      process_fork(regs);
    break;
    case 2:
      process_sleep(regs);
    break;
    case 3:
      printf("%s", regs->ecx);
    break;
    default:
      printf("Unknown User-Interrupt %d\n", regs->eax);
  }
}