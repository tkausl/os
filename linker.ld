OUTPUT_FORMAT(elf32-i386)
ENTRY(_loader)

phys = 0x100000;
offset = 0xC0000000;
virt = offset + phys;

SECTIONS
{
    . = virt;
    KernelStart = .;
    PhysKernelStart = . - offset;

    .text : AT(ADDR(.text) - offset) {
        g_text = .;
        *(.BOOT)
        *(.text)
        *(.rodata)
        . = ALIGN(0x1000);
    }

    .data : AT(ADDR(.data) - offset) {
        g_data = .;
        *(.data)
        . = ALIGN(0x1000);
    }

    .bss : AT(ADDR(.bss) - offset) {
        g_bss = .;
        *(COMMON)
        *(.bss)
        . = ALIGN(0x1000);


        kernel_page_directory = .;
        . = . + 0x3000; /* 3 Pages (12KB) for the page directory */
        kernel_page_tables = .;
        . = . + 0x100000; /* 256 pages for kernel page directory */
        . = ALIGN(0x1000);

    }

    KernelEnd = .;
    PhysKernelEnd = . - offset;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}