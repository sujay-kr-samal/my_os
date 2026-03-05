#include "vga.h"
#include <stdint.h>

static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_color = 0x0F;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0,%1"::"a"(val),"Nd"(port));
}
static void update_cursor(void) {
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));
}
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = ((uint8_t)bg << 4) | (uint8_t)fg;
}
void vga_clear(void) {
    current_color = 0x0F;
    cursor_row = 0; cursor_col = 0;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEMORY[i] = (uint16_t)' ' | (0x0F << 8);
    update_cursor();
}
static void scroll(void) {
    for (int r = 0; r < VGA_HEIGHT-1; r++)
        for (int c = 0; c < VGA_WIDTH; c++)
            VGA_MEMORY[r*VGA_WIDTH+c] = VGA_MEMORY[(r+1)*VGA_WIDTH+c];
    for (int c = 0; c < VGA_WIDTH; c++)
        VGA_MEMORY[(VGA_HEIGHT-1)*VGA_WIDTH+c] = (uint16_t)' ' | (0x00 << 8);
    cursor_row = VGA_HEIGHT-1;
}
void vga_putchar(char c) {
    if (c == '\n') { cursor_col = 0; cursor_row++; }
    else if (c == '\r') { cursor_col = 0; }
    else if (c == '\b') {
        if (cursor_col > 0) cursor_col--;
        else if (cursor_row > 0) { cursor_row--; cursor_col = VGA_WIDTH-1; }
        VGA_MEMORY[cursor_row*VGA_WIDTH+cursor_col] = (uint16_t)' ' | ((uint16_t)current_color << 8);
    } else {
        VGA_MEMORY[cursor_row*VGA_WIDTH+cursor_col] = (uint16_t)(uint8_t)c | ((uint16_t)current_color << 8);
        if (++cursor_col >= VGA_WIDTH) { cursor_col = 0; cursor_row++; }
    }
    if (cursor_row >= VGA_HEIGHT) scroll();
    update_cursor();
}
void vga_print(const char* s) { while (*s) vga_putchar(*s++); }
void vga_print_int(uint64_t n) {
    if (n == 0) { vga_putchar('0'); return; }
    char buf[21]; int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (--i >= 0) vga_putchar(buf[i]);
}
void vga_print_hex(uint32_t n) {
    const char* h = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 7; i >= 0; i--) vga_putchar(h[(n>>(i*4))&0xF]);
}
