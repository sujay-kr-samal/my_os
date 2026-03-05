#ifndef VMM_H
#define VMM_H
#include <stdint.h>
#include <stddef.h>
#define PTE_PRESENT  (1ULL<<0)
#define PTE_WRITABLE (1ULL<<1)
#define PTE_USER     (1ULL<<2)
#define PTE_NOCACHE  (1ULL<<4)
#define PTE_GLOBAL   (1ULL<<8)
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define PT_ENTRIES   512
#define PML4_INDEX(v) (((v)>>39)&0x1FF)
#define PDPT_INDEX(v) (((v)>>30)&0x1FF)
#define PD_INDEX(v)   (((v)>>21)&0x1FF)
#define PT_INDEX(v)   (((v)>>12)&0x1FF)
#define PAGE_OFFSET(v) ((v)&0xFFF)
typedef uint64_t page_table_t[PT_ENTRIES];
typedef struct { page_table_t* pml4; void* regions; uint64_t region_count; } address_space_t;
extern address_space_t kernel_space;
void     vmm_init(void);
int      vmm_map_page(address_space_t* s, uint64_t virt, uint64_t phys, uint64_t flags);
int      vmm_unmap_page(address_space_t* s, uint64_t virt);
uint64_t vmm_get_physical(address_space_t* s, uint64_t virt);
void     vmm_print_map(address_space_t* s);
void     tlb_flush(uint64_t virt);
#endif
