#include "gdt.h"

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)
 
#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed
 
#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR
 
#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD
 
#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR



struct gdt_entry_bits
{
  unsigned int limit_low:16;
  unsigned int base_low : 24;
     //attribute byte split into bitfields
  unsigned int accessed :1;
  unsigned int read_write :1; //readable for code, writable for data
  unsigned int conforming_expand_down :1; //conforming for code, expand down for data
  unsigned int code :1; //1 for code, 0 for data
  unsigned int always_1 :1; //should be 1 for everything but TSS and LDT
  unsigned int DPL :2; //priveledge level
  unsigned int present :1;
     //and now into granularity
  unsigned int limit_high :4;
  unsigned int available :1;
  unsigned int always_0 :1; //should always be 0
  unsigned int big :1; //32bit opcodes for code, uint32_t stack for data
  unsigned int gran :1; //1 to use 4k page addressing, 0 for byte addressing
  unsigned int base_high :8;
} __attribute__((packed));

typedef struct gdt_entry_bits gdt_entry_t;





struct gdt_ptr_struct
{
   uint16_t limit;               // The upper 16 bits of all selector limits.
   uint32_t base;                // The address of the first gdt_entry_t struct.
}
 __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

enum {
   GDT_ENTRY_NULL,
   GDT_ENTRY_KCODE,
   GDT_ENTRY_KDATA,
   GDT_ENTRY_UCODE,
   GDT_ENTRY_UDATA,
   GDT_ENTRY_TSS,
   GDT_ENTRY_NUM
};


uint64_t gdt_entries[GDT_ENTRY_NUM];
gdt_ptr_t   gdt_ptr;

tss_struct_t tss_table;


void gdt_flush(uint32_t ptr);

uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag);

uint64_t tss_build();

void init_gdt()
{
   ZERO_MEM(gdt_entries);
   ZERO_MEM(&tss_table);
   gdt_ptr.limit = (sizeof(uint64_t) * GDT_ENTRY_NUM) - 1;
   gdt_ptr.base  = (uint32_t)&gdt_entries;

   gdt_entries[GDT_ENTRY_NULL] = create_descriptor(0, 0, 0);
   gdt_entries[GDT_ENTRY_KCODE] = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL0));
   gdt_entries[GDT_ENTRY_KDATA] = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL0));
   gdt_entries[GDT_ENTRY_UCODE] = create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL3));
   gdt_entries[GDT_ENTRY_UDATA] = create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL3));

   gdt_entries[GDT_ENTRY_TSS] = tss_build();
   gdt_flush((uint32_t)&gdt_ptr);
}

char tmpcpustack[1024];

uint64_t tss_build() {

   tss_table.ss0 = GDT_ENTRY_KDATA << 3; //0x10; // System-Data Segment is 2 -> 2<<3 == 0x10
   tss_table.esp0 = (uint32_t)&tmpcpustack[1024];


    uint64_t ret = 0;

    gdt_entry_t* g = (gdt_entry_t*)&ret;



   // Firstly, let's compute the base and limit of our entry into the GDT.
   uint32_t base = ((uint32_t)&tss_table) + KERNEL_VIRTUAL_BASE;
   base -= 0xC0000000;
   uint32_t limit = sizeof(tss_table) - 1;
 
   // Now, add our TSS descriptor's address to the GDT.
   g->base_low=base&0xFFFFFF; //isolate bottom 24 bits
   g->accessed=1; //This indicates it's a TSS and not a LDT. This is a changed meaning
   g->read_write=0; //This indicates if the TSS is busy or not. 0 for not busy
   g->conforming_expand_down=0; //always 0 for TSS
   g->code=1; //For TSS this is 1 for 32bit usage, or 0 for 16bit.
   g->always_1=0; //indicate it is a TSS
   g->DPL=3; //same meaning
   g->present=1; //same meaning
   g->limit_high=(limit&0xF0000)>>16; //isolate top nibble
   g->available=0;
   g->always_0=0; //same thing
   g->big=0; //should leave zero according to manuals. No effect
   g->gran=0; //so that our computed GDT limit is in bytes, not pages
   g->base_high=(base&0xFF000000)>>24; //isolate top byte.
   g->limit_low=limit&0xFFFF;

   return ret;
}

void tss_load(unsigned long cpu_num) {
    asm volatile("ltr %%ax": : "a" ((5 + cpu_num)<<3));
}

uint64_t create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    uint64_t descriptor;
 
    // Create the high 32 bit segment
    descriptor  =  limit       & 0x000F0000;         // set limit bits 19:16
    descriptor |= (flag <<  8) & 0x00F0FF00;         // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF;         // set base bits 23:16
    descriptor |=  base        & 0xFF000000;         // set base bits 31:24
 
    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;
 
    // Create the low 32 bit segment
    descriptor |= base  << 16;                       // set base bits 15:0
    descriptor |= limit  & 0x0000FFFF;               // set limit bits 15:0
 
   return descriptor;
}







typedef uint32_t u32int;
typedef uint16_t u16int;
typedef uint8_t u8int;


struct idt_entry_struct
{
   u16int base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   u16int sel;                 // Kernel segment selector.
   u8int  always0;             // This must always be zero.
   u8int  flags;               // More flags. See documentation.
   u16int base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr_struct
{
   u16int limit;
   u32int base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

// These extern directives let us access the addresses of our ASM ISR handlers.
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10 ();
extern void isr11 ();
extern void isr12 ();
extern void isr13 ();
extern void isr14 ();
extern void isr15 ();
extern void isr16 ();
extern void isr17 ();
extern void isr18 ();
extern void isr19 ();
extern void isr20 ();
extern void isr21 ();
extern void isr22 ();
extern void isr23 ();
extern void isr24 ();
extern void isr25 ();
extern void isr26 ();
extern void isr27 ();
extern void isr28 ();
extern void isr29 ();
extern void isr30 ();
extern void isr31();


extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

extern void usr_int_asm();
extern void yield();

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;


void idt_flush(u32int);
void idt_set_gate(u8int num, u32int base, u16int sel, u8int flags);
void idt_set_user_gate(u8int num, u32int base, u16int sel, u8int flags);

#define PIC1      0x20     /* IO base address for master PIC */
#define PIC2      0xA0     /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)

#define ICW1_ICW4 0x01     /* ICW4 (not) needed */
#define ICW1_SINGLE  0x02     /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04     /* Call address interval 4 (8) */
#define ICW1_LEVEL   0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10     /* Initialization - required! */
 
#define ICW4_8086 0x01     /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02     /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08     /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C     /* Buffered mode/master */
#define ICW4_SFNM 0x10     /* Special fully nested (not) */

void io_wait(){
   //for(int i = 0; i < 1000000; ++i);
}

void init_idt()
{
   unsigned char a1, a2;
 
   a1 = inb(PIC1_DATA);                        // save masks
   a2 = inb(PIC2_DATA);
 
   outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
   io_wait();
   outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
   io_wait();
   outb(PIC1_DATA, 0x20);                 // ICW2: Master PIC vector offset
   io_wait();
   outb(PIC2_DATA, 0x28);                 // ICW2: Slave PIC vector offset
   io_wait();
   outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
   io_wait();
   outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
   io_wait();
 
   outb(PIC1_DATA, ICW4_8086);
   io_wait();
   outb(PIC2_DATA, ICW4_8086);
   io_wait();
 
   outb(PIC1_DATA, a1);   // restore saved masks.
   outb(PIC2_DATA, a2);


   idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
   idt_ptr.base  = (u32int)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

   idt_set_gate( 0, (u32int)&isr0 , 0x08, 0x8E);
   idt_set_gate( 1, (u32int)&isr1 , 0x08, 0x8E);
   idt_set_gate( 2, (u32int)&isr2 , 0x08, 0x8E);
   idt_set_gate( 3, (u32int)&isr3 , 0x08, 0x8E);
   idt_set_gate( 4, (u32int)&isr4 , 0x08, 0x8E);
   idt_set_gate( 5, (u32int)&isr5 , 0x08, 0x8E);
   idt_set_gate( 6, (u32int)&isr6 , 0x08, 0x8E);
   idt_set_gate( 7, (u32int)&isr7 , 0x08, 0x8E);
   idt_set_gate( 8, (u32int)&isr8 , 0x08, 0x8E);
   idt_set_gate( 9, (u32int)&isr9 , 0x08, 0x8E);
   idt_set_gate( 10, (u32int)&isr10, 0x08, 0x8E);
   idt_set_gate( 11, (u32int)&isr11 , 0x08, 0x8E);
   idt_set_gate( 12, (u32int)&isr12 , 0x08, 0x8E);
   idt_set_gate( 13, (u32int)&isr13 , 0x08, 0x8E);
   idt_set_gate( 14, (u32int)&isr14 , 0x08, 0x8E);
   idt_set_gate( 15, (u32int)&isr15 , 0x08, 0x8E);
   idt_set_gate( 16, (u32int)&isr16 , 0x08, 0x8E);
   idt_set_gate( 17, (u32int)&isr17 , 0x08, 0x8E);
   idt_set_gate( 18, (u32int)&isr18 , 0x08, 0x8E);
   idt_set_gate( 19, (u32int)&isr19 , 0x08, 0x8E);
   idt_set_gate( 20, (u32int)&isr20 , 0x08, 0x8E);
   idt_set_gate( 21, (u32int)&isr21 , 0x08, 0x8E);
   idt_set_gate( 22, (u32int)&isr22 , 0x08, 0x8E);
   idt_set_gate( 23, (u32int)&isr23 , 0x08, 0x8E);
   idt_set_gate( 24, (u32int)&isr24 , 0x08, 0x8E);
   idt_set_gate( 25, (u32int)&isr25 , 0x08, 0x8E);
   idt_set_gate( 26, (u32int)&isr26 , 0x08, 0x8E);
   idt_set_gate( 27, (u32int)&isr27 , 0x08, 0x8E);
   idt_set_gate( 28, (u32int)&isr28 , 0x08, 0x8E);
   idt_set_gate( 29, (u32int)&isr29 , 0x08, 0x8E);
   idt_set_gate( 30, (u32int)&isr30 , 0x08, 0x8E);
   idt_set_gate( 31, (u32int)&isr31 , 0x08, 0x8E);

   idt_set_gate( 32, (u32int)&irq0 , 0x08, 0x8E);
   idt_set_gate( 33, (u32int)&irq1 , 0x08, 0x8E);
   idt_set_gate( 34, (u32int)&irq2 , 0x08, 0x8E);
   idt_set_gate( 35, (u32int)&irq3 , 0x08, 0x8E);
   idt_set_gate( 36, (u32int)&irq4 , 0x08, 0x8E);
   idt_set_gate( 37, (u32int)&irq5 , 0x08, 0x8E);
   idt_set_gate( 38, (u32int)&irq6 , 0x08, 0x8E);
   idt_set_gate( 39, (u32int)&irq7 , 0x08, 0x8E);
   idt_set_gate( 40, (u32int)&irq8 , 0x08, 0x8E);
   idt_set_gate( 41, (u32int)&irq9 , 0x08, 0x8E);
   idt_set_gate( 42, (u32int)&irq10 , 0x08, 0x8E);
   idt_set_gate( 43, (u32int)&irq11 , 0x08, 0x8E);
   idt_set_gate( 44, (u32int)&irq12 , 0x08, 0x8E);
   idt_set_gate( 45, (u32int)&irq13 , 0x08, 0x8E);
   idt_set_gate( 46, (u32int)&irq14 , 0x08, 0x8E);
   idt_set_gate( 47, (u32int)&irq15 , 0x08, 0x8E);


   idt_set_user_gate( 0x80, (u32int)&usr_int_asm, 0x08, 0x8E);
   idt_set_user_gate( 0x81, (u32int)&yield, 0x08, 0x8E);
   idt_flush((u32int)&idt_ptr);
}

void idt_set_gate(u8int num, u32int base, u16int sel, u8int flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags /* | 0x60 */;
}

void idt_set_user_gate(u8int num, u32int base, u16int sel, u8int flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags  | 0x60;
}