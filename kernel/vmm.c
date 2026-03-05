#include "vmm.h"
#include "memory.h"
#include "vga.h"
address_space_t kernel_space;

void tlb_flush(uint64_t v) { asm volatile("invlpg (%0)"::"r"(v):"memory"); }

static uint64_t* get_pte(page_table_t* pml4, uint64_t v, int create) {
    uint64_t* t = (uint64_t*)pml4;
    int idx[4] = { PML4_INDEX(v), PDPT_INDEX(v), PD_INDEX(v), PT_INDEX(v) };
    for (int level = 0; level < 3; level++) {
        uint64_t e = t[idx[level]];
        if (!(e & PTE_PRESENT)) {
            if (!create) return 0;
            void* np = page_alloc();
            if (!np) return 0;
            uint64_t* p = (uint64_t*)np;
            for (int i = 0; i < 512; i++) p[i] = 0;
            t[idx[level]] = (uint64_t)np | PTE_PRESENT | PTE_WRITABLE;
            e = t[idx[level]];
        }
        t = (uint64_t*)(e & PTE_ADDR_MASK);
    }
    return &t[idx[3]];
}

int vmm_map_page(address_space_t* s, uint64_t v, uint64_t p, uint64_t flags) {
    v &= PAGE_MASK; p &= PAGE_MASK;
    uint64_t* pte = get_pte(s->pml4, v, 1);
    if (!pte) return -1;
    *pte = p | flags | PTE_PRESENT;
    tlb_flush(v);
    return 0;
}
int vmm_unmap_page(address_space_t* s, uint64_t v) {
    uint64_t* pte = get_pte(s->pml4, v, 0);
    if (!pte || !(*pte & PTE_PRESENT)) return -1;
    *pte = 0; tlb_flush(v); return 0;
}
uint64_t vmm_get_physical(address_space_t* s, uint64_t v) {
    uint64_t* pte = get_pte(s->pml4, v, 0);
    if (!pte || !(*pte & PTE_PRESENT)) return 0;
    return (*pte & PTE_ADDR_MASK) | PAGE_OFFSET(v);
}
void vmm_init(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3,%0":"=r"(cr3));
    kernel_space.pml4         = (page_table_t*)(cr3 & PAGE_MASK);
    kernel_space.regions      = 0;
    kernel_space.region_count = 0;
    /* Map 4MB of kernel space (enough for code + BSS + pool + backbuf) */
    for (uint64_t a = 0x10000; a < 0x500000; a += PAGE_SIZE)
        vmm_map_page(&kernel_space, a, a, PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL);
    /* VGA text buffer */
    vmm_map_page(&kernel_space, 0xB8000, 0xB8000, PTE_PRESENT | PTE_WRITABLE | PTE_NOCACHE);
    /* VGA mode 13h framebuffer */
    vmm_map_page(&kernel_space, 0xA0000, 0xA0000, PTE_PRESENT | PTE_WRITABLE | PTE_NOCACHE);
}
void vmm_print_map(address_space_t* s) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n--- PML4 Entries ---\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    uint64_t* pml4 = (uint64_t*)s->pml4;
    for (int i = 0; i < 512; i++) {
        if (!(pml4[i] & PTE_PRESENT)) continue;
        vga_print("  ["); vga_print_int(i); vga_print("] -> ");
        vga_print_hex((uint32_t)(pml4[i] & PTE_ADDR_MASK)); vga_print("\n");
    }
}
