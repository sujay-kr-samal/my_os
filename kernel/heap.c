#include "heap.h"
#include "memory.h"
#include "vga.h"
#define HEAP_START 0x300000
#define HEAP_PAGES 16
#define MAGIC 0xDEADBEEF
typedef struct hdr { uint32_t magic,size; uint8_t free,pad[3]; struct hdr *next,*prev; } hdr_t;
#define HDR sizeof(hdr_t)
static hdr_t* heap = 0;
void heap_init(void) {
    for (int i = 0; i < HEAP_PAGES; i++) page_alloc();
    heap = (hdr_t*)HEAP_START;
    heap->magic = MAGIC;
    heap->size  = HEAP_PAGES * PAGE_SIZE - HDR;
    heap->free  = 1;
    heap->next  = 0;
    heap->prev  = 0;
}
void* kmalloc(size_t sz) {
    if (!sz) return 0;
    sz = (sz+7)&~7;
    for (hdr_t* b = heap; b; b = b->next) {
        if (b->magic != MAGIC) return 0;
        if (!b->free || b->size < sz) continue;
        if (b->size >= sz + HDR + 16) {
            hdr_t* n = (hdr_t*)((uint8_t*)b + HDR + sz);
            n->magic = MAGIC; n->size = b->size - sz - HDR;
            n->free = 1; n->next = b->next; n->prev = b;
            if (b->next) b->next->prev = n;
            b->next = n; b->size = sz;
        }
        b->free = 0;
        return (void*)((uint8_t*)b + HDR);
    }
    return 0;
}
void* kcalloc(size_t n, size_t sz) {
    void* p = kmalloc(n*sz);
    if (p) { uint8_t* b=(uint8_t*)p; for(size_t i=0;i<n*sz;i++) b[i]=0; }
    return p;
}
void kfree(void* ptr) {
    if (!ptr) return;
    hdr_t* b = (hdr_t*)((uint8_t*)ptr - HDR);
    if (b->magic != MAGIC || b->free) return;
    b->free = 1;
    if (b->next && b->next->free) {
        b->size += HDR + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }
    if (b->prev && b->prev->free) {
        b->prev->size += HDR + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}
void heap_print_stats(void) {
    uint32_t used=0,free=0,blocks=0;
    for (hdr_t* b=heap;b;b=b->next) { blocks++; if(b->free) free+=b->size; else used+=b->size; }
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n--- Heap ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Blocks: "); vga_print_int(blocks); vga_print("\n");
    vga_print("  Used  : "); vga_print_int(used);   vga_print(" B\n");
    vga_print("  Free  : "); vga_print_int(free);   vga_print(" B\n");
}
