#include "memory.h"
#include "vga.h"
static uint8_t bitmap[TOTAL_PAGES / 8];
mem_stats_t mem_stats;
static void bset(uint64_t i)  { bitmap[i/8] |=  (1<<(i%8)); }
static void bclr(uint64_t i)  { bitmap[i/8] &= ~(1<<(i%8)); }
static int  bget(uint64_t i)  { return (bitmap[i/8]>>(i%8))&1; }
void memory_init(void) {
    for (int i = 0; i < (int)(TOTAL_PAGES/8); i++) bitmap[i] = 0xFF;
    uint64_t first = FREE_MEM_START / PAGE_SIZE;
    uint64_t last  = TOTAL_MEMORY  / PAGE_SIZE;
    for (uint64_t i = first; i < last; i++) bclr(i);
    mem_stats.total_pages      = TOTAL_PAGES;
    mem_stats.free_pages       = last - first;
    mem_stats.wired_pages      = first;
    mem_stats.active_pages     = 0;
    mem_stats.inactive_pages   = 0;
    mem_stats.compressed_pages = 0;
}
void* page_alloc(void) {
    for (uint64_t i = 0; i < TOTAL_PAGES; i++) {
        if (!bget(i)) {
            bset(i);
            mem_stats.free_pages--;
            mem_stats.active_pages++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return (void*)0;
}
void page_free(void* page) {
    uint64_t idx = (uint64_t)page / PAGE_SIZE;
    if (idx >= TOTAL_PAGES || !bget(idx)) return;
    bclr(idx);
    mem_stats.free_pages++;
    if (mem_stats.active_pages > 0) mem_stats.active_pages--;
}
uint64_t memory_free_bytes(void) { return mem_stats.free_pages * PAGE_SIZE; }
uint64_t memory_used_bytes(void) { return (mem_stats.total_pages - mem_stats.free_pages) * PAGE_SIZE; }
void memory_print_stats(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n--- Physical Memory ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Total     : "); vga_print_int(mem_stats.total_pages*4);      vga_print(" KB\n");
    vga_print("  Free      : "); vga_print_int(mem_stats.free_pages*4);       vga_print(" KB\n");
    vga_print("  Active    : "); vga_print_int(mem_stats.active_pages*4);     vga_print(" KB\n");
    vga_print("  Wired     : "); vga_print_int(mem_stats.wired_pages*4);      vga_print(" KB\n");
    vga_print("  Compressed: "); vga_print_int(mem_stats.compressed_pages*4); vga_print(" KB\n");
}
