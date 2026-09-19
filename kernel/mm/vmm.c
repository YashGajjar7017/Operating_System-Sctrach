/**
 * @file vmm.c
 * @brief Virtual Memory Manager Implementation — Physical Frame Allocator & Page Table Walker
 */

#include "vmm.h"
#include "../../shared/bootinfo.h"

/* ---------------------------------------------------------------------------
 * Global State
 * --------------------------------------------------------------------------- */
PFN_ENTRY *g_pfn_database  = NULL;
uint64_t   g_pfn_count     = 0;
uint32_t   g_pfn_free_head = 0xFFFFFFFF;   /* Sentinel: empty free list */
uint64_t   g_pfn_free_count= 0;
uint64_t   g_pfn_total_bytes = 0;
uint64_t  *g_kernel_pml4   = NULL;

/* Kernel pool virtual bump allocator state */
static uint64_t s_pool_next = KERNEL_POOL_BASE;

/* ---------------------------------------------------------------------------
 * UEFI Memory Type Classification
 * Types >= EfiConventionalMemory(7) that are usable after ExitBootServices:
 *   3  = EfiBootServicesCode
 *   4  = EfiBootServicesData
 *   7  = EfiConventionalMemory
 * --------------------------------------------------------------------------- */
static int is_conventional_memory(uint32_t type) {
    return (type == 3 || type == 4 || type == 7);
}

static int is_reserved_firmware(uint32_t type) {
    /* EfiRuntimeServicesCode=5, EfiRuntimeServicesData=6, EfiMemoryMappedIO=11 */
    return (type == 5 || type == 6 || type == 9 || type == 10 || type == 11 || type == 12);
}

/* ---------------------------------------------------------------------------
 * pfn_database_init — Build PFN database from UEFI memory map
 *
 * We embed the PFN database itself in the first available conventional memory
 * region large enough to hold it. This avoids a chicken-and-egg problem where
 * we need memory to track memory.
 * --------------------------------------------------------------------------- */
void pfn_database_init(const XenithraBootInfo *boot_info) {
    if (!boot_info || !boot_info->memory_map.map) return;

    /* Step 1: Determine total physical memory range to find max PFN */
    uint64_t max_phys = 0;
    uint64_t desc_count = boot_info->memory_map.map_size / boot_info->memory_map.descriptor_size;
    uint8_t *desc_base  = (uint8_t *)boot_info->memory_map.map;

    for (uint64_t i = 0; i < desc_count; i++) {
        XenithraMemoryDescriptor *desc = (XenithraMemoryDescriptor *)(desc_base + i * boot_info->memory_map.descriptor_size);
        uint64_t end = desc->physical_start + desc->number_of_pages * PAGE_SIZE_4K;
        if (end > max_phys) max_phys = end;
    }

    g_pfn_count      = max_phys / PAGE_SIZE_4K;
    g_pfn_total_bytes= max_phys;
    g_pfn_free_head  = 0xFFFFFFFF;
    g_pfn_free_count = 0;

    /* Step 2: Find a region to embed the PFN database */
    uint64_t db_size  = g_pfn_count * sizeof(PFN_ENTRY);
    uint64_t db_pages = (db_size + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K;
    uint64_t db_phys  = 0;

    for (uint64_t i = 0; i < desc_count; i++) {
        XenithraMemoryDescriptor *desc = (XenithraMemoryDescriptor *)(desc_base + i * boot_info->memory_map.descriptor_size);
        if (!is_conventional_memory(desc->type)) continue;
        if (desc->number_of_pages >= db_pages + 1 &&
            desc->physical_start  >= 0x100000) {   /* Above 1MB to avoid real-mode area */
            db_phys = desc->physical_start;
            break;
        }
    }

    if (!db_phys) return; /* Catastrophic: no memory for PFN database */

    /* The PFN database lives at its physical address (identity-mapped) */
    g_pfn_database = (PFN_ENTRY *)(uintptr_t)db_phys;

    /* Step 3: Initialize all PFNs to BAD, then classify from memory map */
    for (uint64_t i = 0; i < g_pfn_count; i++) {
        g_pfn_database[i].state     = PFN_BAD;
        g_pfn_database[i].next_free = 0xFFFFFFFF;
        g_pfn_database[i].ref_count = 0;
        g_pfn_database[i].flags     = 0;
    }

    /* Step 4: Mark regions from memory map */
    for (uint64_t i = 0; i < desc_count; i++) {
        XenithraMemoryDescriptor *desc = (XenithraMemoryDescriptor *)(desc_base + i * boot_info->memory_map.descriptor_size);
        uint32_t pfn_start = (uint32_t)(desc->physical_start / PAGE_SIZE_4K);
        uint32_t pfn_end   = (uint32_t)(pfn_start + desc->number_of_pages);
        if (pfn_end > (uint32_t)g_pfn_count) pfn_end = (uint32_t)g_pfn_count;

        PFN_STATE state = PFN_FREE;
        if (is_reserved_firmware(desc->type))  state = PFN_FIRMWARE;
        else if (is_conventional_memory(desc->type)) state = PFN_FREE;
        else state = PFN_BAD;

        for (uint32_t p = pfn_start; p < pfn_end; p++) {
            g_pfn_database[p].state = state;
        }
    }

    /* Step 5: Mark low memory (0-1MB) as firmware */
    for (uint32_t p = 0; p < 256 && p < (uint32_t)g_pfn_count; p++) {
        g_pfn_database[p].state = PFN_FIRMWARE;
    }

    /* Step 6: Mark kernel binary pages as kernel */
    uint32_t kern_start_pfn = (uint32_t)(boot_info->kernel_phys_base / PAGE_SIZE_4K);
    uint32_t kern_page_count= (uint32_t)((boot_info->kernel_size_bytes + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K);
    for (uint32_t p = kern_start_pfn; p < kern_start_pfn + kern_page_count && p < (uint32_t)g_pfn_count; p++) {
        g_pfn_database[p].state = PFN_KERNEL;
    }

    /* Step 7: Mark PFN database pages themselves as kernel */
    uint32_t db_start_pfn = (uint32_t)(db_phys / PAGE_SIZE_4K);
    for (uint32_t p = db_start_pfn; p < db_start_pfn + db_pages && p < (uint32_t)g_pfn_count; p++) {
        g_pfn_database[p].state = PFN_KERNEL;
    }

    /* Step 8: Build free-list stack (push all PFN_FREE pages, skip page 0) */
    for (uint64_t p = 1; p < g_pfn_count; p++) {
        if (g_pfn_database[p].state == PFN_FREE) {
            g_pfn_database[p].next_free = g_pfn_free_head;
            g_pfn_free_head = (uint32_t)p;
            g_pfn_free_count++;
        }
    }
}

/* ---------------------------------------------------------------------------
 * vmm_alloc_physical — Pop one page from the free-list (O(1))
 * --------------------------------------------------------------------------- */
uint64_t vmm_alloc_physical(void) {
    if (g_pfn_free_head == 0xFFFFFFFF) return 0; /* Out of physical memory */

    uint32_t pfn = g_pfn_free_head;
    g_pfn_free_head = g_pfn_database[pfn].next_free;
    g_pfn_database[pfn].next_free = 0xFFFFFFFF;
    g_pfn_database[pfn].state     = PFN_KERNEL;
    g_pfn_database[pfn].ref_count = 1;
    g_pfn_free_count--;

    /* Zero the page before returning to prevent information leaks */
    uint8_t *page_ptr = (uint8_t *)(uintptr_t)((uint64_t)pfn * PAGE_SIZE_4K);
    __builtin_memset(page_ptr, 0, PAGE_SIZE_4K);

    return (uint64_t)pfn * PAGE_SIZE_4K;
}

/* ---------------------------------------------------------------------------
 * vmm_free_physical — Push one page back onto the free-list (O(1))
 * --------------------------------------------------------------------------- */
void vmm_free_physical(uint64_t phys_addr) {
    uint32_t pfn = (uint32_t)(phys_addr / PAGE_SIZE_4K);
    if (pfn == 0 || pfn >= g_pfn_count) return;
    if (g_pfn_database[pfn].ref_count > 0) g_pfn_database[pfn].ref_count--;
    if (g_pfn_database[pfn].ref_count > 0) return; /* Still referenced */

    g_pfn_database[pfn].state     = PFN_FREE;
    g_pfn_database[pfn].next_free = g_pfn_free_head;
    g_pfn_free_head = pfn;
    g_pfn_free_count++;
}

/* ---------------------------------------------------------------------------
 * vmm_map_page — Walk PML4 → PDPT → PD → PT and install PTE
 *
 * Virtual Address Decoding:
 *   PML4 index = va[47:39]  (bits 47..39)
 *   PDPT index = va[38:30]  (bits 38..30)
 *   PD   index = va[29:21]  (bits 29..21)
 *   PT   index = va[20:12]  (bits 20..12)
 * --------------------------------------------------------------------------- */
int vmm_map_page(uint64_t *pml4, uint64_t va, uint64_t pa, uint64_t flags) {
    /* Extract 9-bit indices from the virtual address */
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;

    /* Level 1: PML4 → PDPT */
    uint64_t *pdpt;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        uint64_t pdpt_phys = vmm_alloc_physical();
        if (!pdpt_phys) return -1;
        pml4[pml4_idx] = pdpt_phys | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
        pdpt = (uint64_t *)(uintptr_t)pdpt_phys;
    } else {
        pdpt = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pml4[pml4_idx]);
    }

    /* Level 2: PDPT → PD */
    uint64_t *pd;
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        uint64_t pd_phys = vmm_alloc_physical();
        if (!pd_phys) return -1;
        pdpt[pdpt_idx] = pd_phys | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
        pd = (uint64_t *)(uintptr_t)pd_phys;
    } else {
        pd = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pdpt[pdpt_idx]);
    }

    /* Level 3: PD → PT */
    uint64_t *pt;
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        uint64_t pt_phys = vmm_alloc_physical();
        if (!pt_phys) return -1;
        pd[pd_idx] = pt_phys | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
        pt = (uint64_t *)(uintptr_t)pt_phys;
    } else {
        /* Clear huge-page bit if this PD entry was a 2MB page */
        pd[pd_idx] &= ~PTE_HUGE;
        pt = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pd[pd_idx]);
    }

    /* Level 4: Install the actual PTE */
    pt[pt_idx] = (pa & PAGE_MASK_4K) | flags | PTE_PRESENT;

    /* Invalidate this VA's TLB entry */
    vmm_invlpg(va);

    return 0;
}

/* ---------------------------------------------------------------------------
 * vmm_unmap_page — Remove a PTE and invalidate TLB
 * --------------------------------------------------------------------------- */
void vmm_unmap_page(uint64_t *pml4, uint64_t va) {
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return;
    uint64_t *pdpt = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pml4[pml4_idx]);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;
    uint64_t *pd   = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pdpt[pdpt_idx]);
    if (!(pd[pd_idx] & PTE_PRESENT)) return;
    uint64_t *pt   = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pd[pd_idx]);

    pt[pt_idx] = 0;
    vmm_invlpg(va);
}

/* ---------------------------------------------------------------------------
 * vmm_virt_to_phys — Page table walk to resolve VA → PA
 * --------------------------------------------------------------------------- */
uint64_t vmm_virt_to_phys(uint64_t *pml4, uint64_t va) {
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return 0;
    uint64_t *pdpt = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pml4[pml4_idx]);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;
    uint64_t *pd   = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pdpt[pdpt_idx]);
    if (!(pd[pd_idx]   & PTE_PRESENT)) return 0;
    /* Check for 2MB huge page */
    if (pd[pd_idx] & PTE_HUGE) {
        return PTE_PHYS_ADDR(pd[pd_idx]) + (va & (PAGE_SIZE_2M - 1));
    }
    uint64_t *pt   = (uint64_t *)(uintptr_t)PTE_PHYS_ADDR(pd[pd_idx]);
    if (!(pt[pt_idx]   & PTE_PRESENT)) return 0;

    return PTE_PHYS_ADDR(pt[pt_idx]) + (va & (PAGE_SIZE_4K - 1));
}

/* ---------------------------------------------------------------------------
 * vmm_init — Initialize VMM with the boot page tables from the bootloader
 * --------------------------------------------------------------------------- */
void vmm_init(const XenithraBootInfo *boot_info) {
    if (!boot_info) return;

    /* Initialize physical frame database first */
    pfn_database_init(boot_info);

    /* Get CR3 — the bootloader's PML4 physical address (identity-mapped) */
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    g_kernel_pml4 = (uint64_t *)(uintptr_t)cr3;

    /* Reset the virtual bump allocator for the kernel pool region */
    s_pool_next = KERNEL_POOL_BASE;
}

/* ---------------------------------------------------------------------------
 * vmm_alloc_pages — Allocate N virtual pages backed by physical frames
 * --------------------------------------------------------------------------- */
uint64_t vmm_alloc_pages(uint64_t n, uint64_t flags) {
    if (!n || !g_kernel_pml4) return 0;
    if (g_pfn_free_count < n) return 0;  /* Not enough physical memory */

    uint64_t va_base = s_pool_next;
    s_pool_next += n * PAGE_SIZE_4K;

    for (uint64_t i = 0; i < n; i++) {
        uint64_t phys = vmm_alloc_physical();
        if (!phys) {
            /* Rollback already-mapped pages */
            for (uint64_t j = 0; j < i; j++) {
                uint64_t mapped_va = va_base + j * PAGE_SIZE_4K;
                uint64_t mapped_pa = vmm_virt_to_phys(g_kernel_pml4, mapped_va);
                if (mapped_pa) vmm_free_physical(mapped_pa);
                vmm_unmap_page(g_kernel_pml4, mapped_va);
            }
            s_pool_next = va_base;
            return 0;
        }
        if (vmm_map_page(g_kernel_pml4, va_base + i * PAGE_SIZE_4K, phys, flags) != 0) {
            vmm_free_physical(phys);
            s_pool_next = va_base;
            return 0;
        }
    }

    return va_base;
}

/* ---------------------------------------------------------------------------
 * vmm_free_pages — Free N virtual pages and their backing physical frames
 * --------------------------------------------------------------------------- */
void vmm_free_pages(uint64_t va, uint64_t n) {
    if (!va || !n || !g_kernel_pml4) return;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t page_va = va + i * PAGE_SIZE_4K;
        uint64_t page_pa = vmm_virt_to_phys(g_kernel_pml4, page_va);
        if (page_pa) vmm_free_physical(page_pa);
        vmm_unmap_page(g_kernel_pml4, page_va);
    }
}
