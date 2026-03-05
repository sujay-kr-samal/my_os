#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include <stddef.h>
#define PAGE_SIZE      4096
#define PAGE_MASK      (~(uint64_t)(PAGE_SIZE-1))
#define TOTAL_MEMORY   (64ULL*1024*1024)
#define TOTAL_PAGES    (TOTAL_MEMORY/PAGE_SIZE)
#define FREE_MEM_START 0x20000
typedef struct { uint64_t total_pages,free_pages,active_pages,inactive_pages,wired_pages,compressed_pages; } mem_stats_t;
extern mem_stats_t mem_stats;
void     memory_init(void);
void*    page_alloc(void);
void     page_free(void* page);
void     memory_print_stats(void);
uint64_t memory_free_bytes(void);
uint64_t memory_used_bytes(void);
#endif
