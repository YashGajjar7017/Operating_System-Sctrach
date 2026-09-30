/**
 * @file vmm.h
 * @brief Virtual Memory Manager & Physical Frame Allocator for Xenithra OS
 *
 * Implements the three-tier memory management architecture:
 *
 *   Tier 1: Physical Frame Allocator (PFN Database)
 *     - Tracks the state of every 4KB physical page frame
 *     - O(1) allocation via a free-list stack (LIFO, cache-friendly)
 *     - Populated from XenithraBootInfo.memory_map at boot
 *
 *   Tier 2: Virtual Address Space Manager
 *     - Walks and modifies 4-level page table hierarchies
 *     - Maps/unmaps virtual → physical page entries
 *     - Handles TLB invalidation (INVLPG) after page table changes
 *
 *   Tier 3: Virtual Region Allocator
 *     - Tracks committed virtual memory regions per address space
 *     - Provides vmm_alloc_pages() and vmm_free_pages() to callers
 *
 * 4-Level Page Table Walk (Intel IA-32e, 48-bit canonical VA):
 *
 *   Virtual Address [63:0]:
 *   [63:48] Sign extension (must match bit 47)
 *   [47:39] PML4 index (9 bits → 512 entries)
 *   [38:30] PDPT  index (9 bits → 512 entries)
 *   [29:21] PD    index (9 bits → 512 entries)
 *   [20:12] PT    index (9 bits → 512 entries)
 *   [11:0]  Page  offset (12 bits → 4096-byte page)
 */

#ifndef _KERNEL_MM_VMM_H_
#define _KERNEL_MM_VMM_H_

#include <stdint.h>
#include <stddef.h>
#include "../../shared/bootinfo.h"

/* ---------------------------------------------------------------------------
 * Page Size Constants
 * --------------------------------------------------------------------------- */
#define PAGE_SIZE_4K         0x1000ULL          /* 4 KB */
#define PAGE_SIZE_2M         0x200000ULL         /* 2 MB (huge page) */
#define PAGE_SIZE_1G         0x40000000ULL       /* 1 GB (giant page) */
#define PAGE_MASK_4K         (~(PAGE_SIZE_4K - 1))
#define PAGE_ALIGN_UP(x)     (((uint64_t)(x) + PAGE_SIZE_4K - 1) & PAGE_MASK_4K)
#define PAGE_ALIGN_DOWN(x)   ((uint64_t)(x) & PAGE_MASK_4K)

/* Physical address of the PML4 (set by bootloader, accessible via CR3) */
#define KERNEL_PML4_BASE     0x0000000000000000ULL  /* Identity-mapped root */

/* Kernel higher-half virtual base (matches bootloader linker.ld) */
#define KERNEL_VIRT_BASE     0xFFFFFFFF80000000ULL

/* Non-Paged Pool virtual base (128 MB kernel heap region) */
#define KERNEL_POOL_BASE     0xFFFFFF8000000000ULL
#define KERNEL_POOL_SIZE     (128ULL * 1024 * 1024)   /* 128 MB */

/* ---------------------------------------------------------------------------
 * Page Table Entry (PTE) Flags — Intel SDM Vol.3A §4.5
 * --------------------------------------------------------------------------- */
#define PTE_PRESENT          (1ULL << 0)   /* P: Page present in physical memory */
#define PTE_WRITABLE         (1ULL << 1)   /* R/W: Read/Write permission */
#define PTE_USER             (1ULL << 2)   /* U/S: User-mode accessible (Ring 3) */
#define PTE_WRITE_THROUGH    (1ULL << 3)   /* PWT: Write-through caching */
#define PTE_CACHE_DISABLE    (1ULL << 4)   /* PCD: Disable cache for MMIO */
#define PTE_ACCESSED         (1ULL << 5)   /* A: CPU sets when page is accessed */
#define PTE_DIRTY            (1ULL << 6)   /* D: CPU sets when page is written */
#define PTE_HUGE             (1ULL << 7)   /* PS: 2MB or 1GB page (in PD/PDPT) */
#define PTE_GLOBAL           (1ULL << 8)   /* G: Global — not flushed on CR3 reload */
#define PTE_NX               (1ULL << 63)  /* XD: Execute-Disable (NX bit) */

/* Common flag combinations */
#define PTE_KERNEL_RW        (PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL)
#define PTE_KERNEL_RO        (PTE_PRESENT | PTE_GLOBAL   | PTE_NX)
#define PTE_USER_RW          (PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX)
#define PTE_USER_RO          (PTE_PRESENT | PTE_USER     | PTE_NX)
#define PTE_MMIO             (PTE_PRESENT | PTE_WRITABLE | PTE_CACHE_DISABLE | PTE_WRITE_THROUGH | PTE_GLOBAL | PTE_NX)

/* Extract physical address from a page table entry (mask out flag bits) */
#define PTE_PHYS_ADDR(pte)   ((pte) & 0x000FFFFFFFFFF000ULL)

/* ---------------------------------------------------------------------------
 * PFN (Page Frame Number) Database — tracks state of every physical page
 * --------------------------------------------------------------------------- */
typedef enum _PFN_STATE {
    PFN_FREE       = 0,   /* Available for allocation */
    PFN_KERNEL     = 1,   /* Kernel code/data/stack */
    PFN_USER       = 2,   /* User process pages */
    PFN_MMIO       = 3,   /* Memory-mapped I/O region */
    PFN_FIRMWARE   = 4,   /* UEFI runtime/firmware reserved */
    PFN_PAGETABLE  = 5,   /* Used as a page table page itself */
    PFN_BAD        = 6,   /* Defective or non-usable */
} PFN_STATE;

typedef struct _PFN_ENTRY {
    uint32_t    next_free;  /* Index of next free PFN (free-list chain) */
    uint8_t     state;      /* PFN_STATE */
    uint8_t     ref_count;  /* Reference count (for shared pages / COW) */
    uint16_t    flags;      /* Reserved for future use */
} PFN_ENTRY;

/* ---------------------------------------------------------------------------
 * Virtual Memory Region Descriptor
 * --------------------------------------------------------------------------- */
typedef struct _VMM_REGION {
    uint64_t    virt_base;  /* Region virtual start address (page-aligned) */
    uint64_t    phys_base;  /* Region physical start address (page-aligned) */
    uint64_t    size;       /* Region size in bytes */
    uint64_t    flags;      /* PTE flags (PTE_KERNEL_RW, PTE_USER_RW, etc.) */
    const char *name;       /* Human-readable debug name */
    struct _VMM_REGION *next;
} VMM_REGION;

/* ---------------------------------------------------------------------------
 * Global VMM State
 * --------------------------------------------------------------------------- */
extern PFN_ENTRY *g_pfn_database;     /* PFN array, one entry per physical page */
extern uint64_t   g_pfn_count;        /* Total number of page frames */
extern uint32_t   g_pfn_free_head;    /* Free-list head index (LIFO stack) */
extern uint64_t   g_pfn_free_count;   /* Number of free page frames */
extern uint64_t   g_pfn_total_bytes;  /* Total physical memory (bytes) */
extern uint64_t  *g_kernel_pml4;      /* Virtual address of the kernel PML4 */

/* ---------------------------------------------------------------------------
 * Physical Frame Allocator API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the PFN database from the UEFI memory map in XenithraBootInfo.
 *        Builds the free-list stack for all EfiConventionalMemory regions.
 *        Must be called as part of Phase 0 kernel init, after GDT/IDT setup.
 * @param boot_info Pointer to the boot information block from the bootloader.
 */
void pfn_database_init(const XenithraBootInfo *boot_info);

/**
 * @brief Allocate a single 4KB physical page frame.
 * @return Physical page address, or 0 if out of physical memory.
 */
uint64_t vmm_alloc_physical(void);

/**
 * @brief Free a previously allocated physical page frame.
 * @param phys_addr Physical address of the 4KB page to free.
 */
void vmm_free_physical(uint64_t phys_addr);

static inline uint64_t vmm_get_total_kb(void) {
    return g_pfn_total_bytes / 1024;
}

static inline uint64_t vmm_get_free_kb(void) {
    return (g_pfn_free_count * PAGE_SIZE_4K) / 1024;
}

/* ---------------------------------------------------------------------------
 * Virtual Memory Mapping API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the VMM using the boot page tables from the bootloader.
 *        Maps kernel pool region into the virtual address space.
 * @param boot_info Pointer to the boot information block.
 */
void vmm_init(const XenithraBootInfo *boot_info);

/**
 * @brief Map a single 4KB virtual page to a physical frame.
 *        Allocates intermediate page table pages (PDPT, PD, PT) as needed.
 * @param pml4   Pointer to the PML4 table (virtual address).
 * @param va     Virtual address to map (must be 4KB-aligned).
 * @param pa     Physical address to map to (must be 4KB-aligned).
 * @param flags  Combination of PTE_* flags.
 * @return 0 on success, -1 on failure (out of memory for page tables).
 */
int vmm_map_page(uint64_t *pml4, uint64_t va, uint64_t pa, uint64_t flags);

/**
 * @brief Unmap a single 4KB virtual page and invalidate TLB entry.
 * @param pml4   Pointer to the PML4 table.
 * @param va     Virtual address to unmap.
 */
void vmm_unmap_page(uint64_t *pml4, uint64_t va);

/**
 * @brief Resolve a virtual address to its physical address via page table walk.
 * @param pml4   Pointer to the PML4 table.
 * @param va     Virtual address to resolve.
 * @return Physical address, or 0 if the mapping does not exist.
 */
uint64_t vmm_virt_to_phys(uint64_t *pml4, uint64_t va);

/**
 * @brief Allocate `n` contiguous virtual pages in the kernel address space.
 *        Maps them to freshly-allocated physical frames.
 * @param n     Number of 4KB pages to allocate.
 * @param flags PTE flags for the mapping.
 * @return Virtual address of the allocation, or 0 on failure.
 */
uint64_t vmm_alloc_pages(uint64_t n, uint64_t flags);

/**
 * @brief Free virtual pages previously allocated with vmm_alloc_pages().
 * @param va    Virtual base address of the region.
 * @param n     Number of 4KB pages to free.
 */
void vmm_free_pages(uint64_t va, uint64_t n);

/**
 * @brief TLB full flush — reload CR3 to invalidate all non-global TLB entries.
 */
static inline void vmm_flush_tlb(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

/**
 * @brief Invalidate a single TLB entry for the given virtual address.
 *        More efficient than a full CR3 reload for single-page changes.
 */
static inline void vmm_invlpg(uint64_t va) {
    __asm__ volatile ("invlpg (%0)" :: "r"(va) : "memory");
}

#endif /* _KERNEL_MM_VMM_H_ */
