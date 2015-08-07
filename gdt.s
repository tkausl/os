.section .text
.global gdt_flush
.global idt_flush
.global jump_to_user
.global timer_int
.type gdt_flush, @function
.type idt_flush, @function
.type jump_to_user, @function
.type timer_int, @function
jump_to_user:
	mov $0x23, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	mov %esp, %eax
	pushl $0x23
	push %eax
	pushf
	pushl $0x1B
	push $go_ahead
	iret
go_ahead:
	ret


gdt_flush:
	movl 4(%esp), %eax
	lgdt (%eax)
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	jmp $0x08, $gdt_flush_jmp
gdt_flush_jmp:
	mov $0x2B, %ax
	ltr %ax
	ret

idt_flush:
	mov 4(%esp), %eax
	lidt (%eax)
	ret



.macro ISR_NOERRCODE num
  .global isr\num
  .type isr\num, @function
  isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
  .global isr\num
  .type isr\num, @function
  isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

.macro IRQ num
  .global irq\num
  .type irq\num, @function
  irq\num:
  	cli
  	push $0
  	push $\num
    jmp irq_common_stub
.endm

.global usr_int_asm
.type usr_int_asm, @function
usr_int_asm:
	pusha

	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp

	call usr_int

	add $4, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	iret	


irq_common_stub:

	pusha

	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp

	call irq_handler

	add $4, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp
	iret	

IRQ   1
IRQ   2
IRQ   3
IRQ   4
IRQ   5
IRQ   6
IRQ   7
IRQ   8
IRQ   9
IRQ   10
IRQ   11
IRQ   12
IRQ   13
IRQ   14
IRQ   15

isr_common_stub:
	xchg %bx, %bx
	pusha

	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp

	call isr_handler

	add $4, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	add $8, %esp
	iret


ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

.global irq0
.type irq0, @function
irq0: //Timer Interrupt! Scheduling please!
	cli
	pusha

	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp

	call timer_int

	mov %eax, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	iret

.global yield
.type yield, @function
yield: //Yielded
	cli
	pusha

	mov %ds, %ax
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	push %esp

	call process_yield

	mov %eax, %esp

	pop %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	popa
	iret
