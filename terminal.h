#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"

enum vga_color {
	COLOR_BLACK = 0,
	COLOR_BLUE = 1,
	COLOR_GREEN = 2,
	COLOR_CYAN = 3,
	COLOR_RED = 4,
	COLOR_MAGENTA = 5,
	COLOR_BROWN = 6,
	COLOR_LIGHT_GREY = 7,
	COLOR_DARK_GREY = 8,
	COLOR_LIGHT_BLUE = 9,
	COLOR_LIGHT_GREEN = 10,
	COLOR_LIGHT_CYAN = 11,
	COLOR_LIGHT_RED = 12,
	COLOR_LIGHT_MAGENTA = 13,
	COLOR_LIGHT_BROWN = 14,
	COLOR_WHITE = 15,
};

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static const size_t TAB_SIZE = 4;

inline uint8_t make_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

inline uint16_t make_vgaentry(char c, uint8_t color) {
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}

size_t strlen(const char* str);

void terminal_initialize();

void terminal_setcolor(uint8_t color);

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

void terminal_break();

void terminal_advance();

void terminal_tab();

void terminal_putchar(char c);

void itoa(char *buf, int base, int d);

void printf (const char *format, ...);

void update_cursor(int row, int col);

#endif