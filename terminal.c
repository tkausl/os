#include "terminal.h"


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


size_t strlen(const char* str) {
	size_t ret = 0;
	while(str[ret] != 0)
		ret++;
	return ret;
}

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize() {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xC00B8000;
	for(size_t y = 0; y < VGA_HEIGHT; y++) {
		for(size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = make_vgaentry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = make_vgaentry(c, color);
}

void terminal_break() {
	terminal_column = 0;
	if(++terminal_row == VGA_HEIGHT) {
		--terminal_row;
    memcpy(terminal_buffer, terminal_buffer + VGA_WIDTH, (VGA_WIDTH * (VGA_HEIGHT - 1)) * 2);
    memset(terminal_buffer + (VGA_WIDTH * (VGA_HEIGHT-1)), 0, VGA_WIDTH * 2);
	}
}

void terminal_advance() {
	if(++terminal_column == VGA_WIDTH) {
		terminal_break();
	}	
}

void terminal_tab() {
	size_t adv = TAB_SIZE - (terminal_column % 4);
	terminal_column += adv;
}

void terminal_putchar(char c) {
	switch(c) {
		case '\n':
			terminal_break();
		break;
		case '\t':
			terminal_tab();
		break;
		default:
			terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
			terminal_advance();
	}

}

void itoa (char *buf, int base, int d)
     {
       char *p = buf;
       char *p1, *p2;
       unsigned long ud = d;
       int divisor = 10;
     
       /* If %d is specified and D is minus, put `-' in the head. */
       if (base == 'd' && d < 0)
         {
           *p++ = '-';
           buf++;
           ud = -d;
         }
       else if (base == 'x')
         divisor = 16;
     
       /* Divide UD by DIVISOR until UD == 0. */
       do
         {
           int remainder = ud % divisor;
     
           *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
         }
       while (ud /= divisor);
     

       /* Terminate BUF. */
       *p = 0;
     
       /* Reverse BUF. */
       p1 = buf;
       p2 = p - 1;
       while (p1 < p2)
         {
           char tmp = *p1;
           *p1 = *p2;
           *p2 = tmp;
           p1++;
           p2--;
         }
     }

void printf (const char *format, ...)
     {
       char **arg = (char **) &format;
       int c;
       char buf[20];
     
       arg++;
     
       while ((c = *format++) != 0)
         {
           if (c != '%')
             terminal_putchar (c);
           else
             {
               char *p;
     
               c = *format++;
               switch (c)
                 {
                 case 'd':
                 case 'u':
                 case 'x':
                   itoa (buf, c, *((int *) arg++));
                   p = buf;
                   goto string;
                   break;
     
                 case 's':
                   p = *arg++;
                   if (! p)
                     p = "(null)";
     
                 string:
                   while (*p)
                     terminal_putchar (*p++);
                   break;
     
                 default:
                   terminal_putchar (*((int *) arg++));
                   break;
                 }
             }
         }
     }

 void update_cursor(int row, int col)
 {
    unsigned short position=(row*80) + col;
 
    // cursor LOW port to vga INDEX register
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position&0xFF));
    // cursor HIGH port to vga INDEX register
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position>>8)&0xFF));
 }

 void convert(unsigned long num, char* str) {
 	int l = 0;
 	while(num > 1024) {
 		num /= 1024;
 		l++;
 	}
 	itoa(str, 10, num);
 	int pos = strlen(str);
 	str[pos++] = ' ';
 	str[pos++] = "BKMGT"[l];
 	str[pos++] = 0;
 }