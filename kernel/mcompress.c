#include "mcompress.h"
#include "compress.h"
#include "vmm.h"
#include "memory.h"
#include "vga.h"

/* Pool as static array - guaranteed to be in kernel BSS, always valid */
static uint8_t pool_storage[COMPRESSED_POOL_SIZE];
static uint8_t* pool = pool_storage;
static uint32_t pool_ptr = 0;
static compressed_page_t table[MAX_COMPRESSED_PAGES];
compress_stats_t compress_stats;
static uint8_t cbuf[COMPRESS_BOUND(PAGE_SIZE)];
static uint8_t dbuf[PAGE_SIZE];

void mcompress_init(void) {
    for(int i=0;i<MAX_COMPRESSED_PAGES;i++) table[i].in_use=0;
    pool_ptr=0;
    compress_stats.compression_ratio_x10=10;
}

static int free_slot(void) {
    for(int i=0;i<MAX_COMPRESSED_PAGES;i++) if(!table[i].in_use) return i;
    return -1;
}
static int find_slot(uint64_t v) {
    for(int i=0;i<MAX_COMPRESSED_PAGES;i++) if(table[i].in_use&&table[i].virt_addr==v) return i;
    return -1;
}

int mcompress_page(address_space_t* s, uint64_t v) {
    v &= PAGE_MASK;
    uint64_t phys = vmm_get_physical(s, v);
    if (!phys) return -1;
    int slot = free_slot(); if (slot<0) return -2;
    if (pool_ptr + COMPRESS_BOUND(PAGE_SIZE) > COMPRESSED_POOL_SIZE) return -3;
    int cs = compress_page((uint8_t*)v, PAGE_SIZE, cbuf, sizeof(cbuf));
    if (cs == COMPRESS_FAIL) return -4;
    uint8_t* dst = pool + pool_ptr;
    for(int i=0;i<cs;i++) dst[i]=cbuf[i];
    table[slot].virt_addr=v; table[slot].phys_was=phys;
    table[slot].pool_offset=pool_ptr; table[slot].compressed_size=cs;
    table[slot].original_size=PAGE_SIZE; table[slot].in_use=1;
    pool_ptr+=cs;
    vmm_unmap_page(s,v);
    page_free((void*)phys);
    int saved=PAGE_SIZE-cs;
    compress_stats.pages_compressed++;
    compress_stats.pages_currently_compressed++;
    compress_stats.bytes_saved+=saved;
    compress_stats.pool_bytes_used=pool_ptr;
    if(pool_ptr>0) compress_stats.compression_ratio_x10=
        (compress_stats.pages_currently_compressed*PAGE_SIZE*10)/pool_ptr;
    return saved;
}

int mdecompress_page(address_space_t* s, uint64_t v) {
    v &= PAGE_MASK;
    int slot=find_slot(v); if(slot<0) return -1;
    void* np=page_alloc(); if(!np) return -2;
    int r=decompress_page(pool+table[slot].pool_offset, table[slot].compressed_size, dbuf, PAGE_SIZE);
    if(r!=PAGE_SIZE){ page_free(np); return -3; }
    uint8_t* pp=(uint8_t*)np;
    for(int i=0;i<PAGE_SIZE;i++) pp[i]=dbuf[i];
    vmm_map_page(s,v,(uint64_t)np,PTE_PRESENT|PTE_WRITABLE);
    table[slot].in_use=0;
    compress_stats.pages_decompressed++;
    compress_stats.pages_currently_compressed--;
    compress_stats.pool_bytes_used-=table[slot].compressed_size;
    return 0;
}

int mcompress_is_compressed(uint64_t v) { return find_slot(v&PAGE_MASK)>=0; }

void mcompress_print_stats(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n--- Compression Stats ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Compressed  : "); vga_print_int(compress_stats.pages_compressed);           vga_print(" pages\n");
    vga_print("  Decompressed: "); vga_print_int(compress_stats.pages_decompressed);          vga_print(" pages\n");
    vga_print("  In pool     : "); vga_print_int(compress_stats.pages_currently_compressed);  vga_print(" pages\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("  RAM saved   : "); vga_print_int(compress_stats.bytes_saved/1024);            vga_print(" KB\n");
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("  Ratio       : ");
    vga_print_int(compress_stats.compression_ratio_x10/10); vga_print(".");
    vga_print_int(compress_stats.compression_ratio_x10%10); vga_print("x\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}