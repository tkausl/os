CC 			= i686-elf-gcc
CCFLAGS 	= -std=gnu99 -ffreestanding -O2 -Wall -Wextra

NASM 		= nasm
NASMFLAGS 	= -felf

LD 			= i686-elf-gcc
LDFLAGS 	= -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib -lgcc


ALL 		= boot.o gdt.o kernel.o memory.o pmm.o vmm.o scheduler.o terminal.o elf.o

build: myos.bin
	i686-elf-gcc -c -std=gnu99 -O2 -Wall -Wextra -nostdlib -lgcc -ffreestanding helloworld.c
	i686-elf-gcc -c -std=gnu99 -O2 -Wall -Wextra -nostdlib -lgcc -ffreestanding stdlib.s
	i686-elf-gcc -std=gnu99 -O2 -nostdlib -lgcc -ffreestanding  stdlib.o helloworld.o
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp myos.bin iso/boot/
	cp a.out iso/initrd
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o myos.iso iso

myos.bin: $(ALL)
	i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib $(ALL) -lgcc

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

boot.o: boot.s
	$(NASM) $(NASMFLAGS) boot.s

gdt.o: gdt.s
	i686-elf-as gdt.s -o gdt.o


clean:
	rm *.o
	rm *.iso
	rm *.bin
	rm -rf iso
