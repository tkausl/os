.section .text
.global _start

_start:
	call main
	mov %eax, %ecx
	int $0x80
