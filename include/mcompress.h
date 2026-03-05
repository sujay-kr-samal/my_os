#ifndef MCOMPRESS_H
#define MCOMPRESS_H
#include <stdint.h>
#include <stddef.h>
#include "vmm.h"
#define MAX_COMPRESSED_PAGES 256
#define COMPRESSED_POOL_SIZE (2*1024*1024)
#define COMPRESSED_POOL_BASE 0x400000
typedef struct {
    uint64_t virt_addr, phys_was;
    uint32_t pool_offset, compressed_size, original_size;
    uint8_t  in_use;
} compressed_page_t;
typedef struct {
    uint64_t pages_compressed, pages_decompressed, pages_currently_compressed;
    uint64_t bytes_saved, pool_bytes_used, compression_ratio_x10;
} compress_stats_t;
extern compress_stats_t compress_stats;
void mcompress_init(void);
int  mcompress_page(address_space_t* space, uint64_t virt_addr);
int  mdecompress_page(address_space_t* space, uint64_t virt_addr);
int  mcompress_is_compressed(uint64_t virt_addr);
void mcompress_print_stats(void);
#endif
