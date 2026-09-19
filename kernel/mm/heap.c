/**
 * @file heap.c
 * @brief Kernel Non-Paged Pool — Boundary-Tag Coalescing Free-List Allocator
 *
 * Design: Each allocation is preceded by a header tag and followed by a footer
 * tag, both containing the block size and a free/used flag. This allows O(1)
 * coalescing of adjacent free blocks on kfree(), preventing heap fragmentation.
 *
 * Block layout in memory:
 *
 *   [HEAP_BLOCK_HEADER]  (16 bytes: size + flags + magic)
 *   [User data          ]  (requested bytes, 16-byte aligned)
 *   [HEAP_BLOCK_FOOTER  ]  (8 bytes:  size + flags)
 *
 * Free list: doubly-linked list of free blocks stored inside the block's
 * user-data area when the block is free (similar to Doug Lea's dlmalloc).
 */

#include "heap.h"

#define HEAP_MAGIC      0xDEADC0DEUL   /* Corruption detection canary */
#define HEAP_ALIGN      16             /* Minimum alignment (FXSAVE requirement) */
#define HEAP_MIN_BLOCK  (sizeof(HeapHeader) + sizeof(HeapFooter) + HEAP_ALIGN)

/* ---------------------------------------------------------------------------
 * Block Header (16 bytes, precedes every allocation)
 * --------------------------------------------------------------------------- */
typedef struct _HeapHeader {
    uint32_t magic;         /* HEAP_MAGIC — corruption guard */
    uint32_t flags;         /* Bit 0: 0=free, 1=used */
    uint64_t size;          /* Total block size (including header + footer) */
} HeapHeader;

/* ---------------------------------------------------------------------------
 * Block Footer (8 bytes, follows every allocation)
 * --------------------------------------------------------------------------- */
typedef struct _HeapFooter {
    uint64_t size;          /* Mirror of header.size for backward traversal */
} HeapFooter;

/* ---------------------------------------------------------------------------
 * Free List Node (overlays user-data area when block is free)
 * --------------------------------------------------------------------------- */
typedef struct _FreeNode {
    struct _FreeNode *prev;
    struct _FreeNode *next;
} FreeNode;

/* ---------------------------------------------------------------------------
 * Global Heap State
 * --------------------------------------------------------------------------- */
static uint8_t   *s_heap_base  = NULL;
static uint8_t   *s_heap_end   = NULL;
static uint64_t   s_heap_size  = 0;
static uint64_t   s_free_bytes = 0;
static FreeNode  *s_free_list  = NULL;   /* Head of the free-list */

/* ---------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------- */
static inline HeapHeader *block_header(void *user_ptr) {
    return (HeapHeader *)((uint8_t *)user_ptr - sizeof(HeapHeader));
}

static inline HeapFooter *block_footer(HeapHeader *hdr) {
    return (HeapFooter *)((uint8_t *)hdr + hdr->size - sizeof(HeapFooter));
}

static inline HeapHeader *next_block(HeapHeader *hdr) {
    return (HeapHeader *)((uint8_t *)hdr + hdr->size);
}

static inline HeapHeader *prev_block(HeapHeader *hdr) {
    HeapFooter *prev_foot = (HeapFooter *)((uint8_t *)hdr - sizeof(HeapFooter));
    return (HeapHeader *)((uint8_t *)hdr - prev_foot->size);
}

static inline void free_list_insert(HeapHeader *hdr) {
    FreeNode *node = (FreeNode *)((uint8_t *)hdr + sizeof(HeapHeader));
    node->next = s_free_list;
    node->prev = NULL;
    if (s_free_list) s_free_list->prev = node;
    s_free_list = node;
}

static inline void free_list_remove(HeapHeader *hdr) {
    FreeNode *node = (FreeNode *)((uint8_t *)hdr + sizeof(HeapHeader));
    if (node->prev) node->prev->next = node->next;
    else s_free_list = node->next;
    if (node->next) node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
}

/* ---------------------------------------------------------------------------
 * kheap_init — Bootstrap the heap from a known virtual region
 * --------------------------------------------------------------------------- */
void kheap_init(uint64_t base, uint64_t size) {
    if (!base || size < HEAP_MIN_BLOCK * 4) return;

    s_heap_base  = (uint8_t *)(uintptr_t)base;
    s_heap_end   = s_heap_base + size;
    s_heap_size  = size;
    s_free_list  = NULL;

    /* Create a single large free block spanning the entire pool */
    HeapHeader *hdr = (HeapHeader *)s_heap_base;
    hdr->magic = HEAP_MAGIC;
    hdr->flags = 0;   /* Free */
    hdr->size  = size;

    HeapFooter *ftr = block_footer(hdr);
    ftr->size = size;

    free_list_insert(hdr);
    s_free_bytes = size - sizeof(HeapHeader) - sizeof(HeapFooter);
}

/* ---------------------------------------------------------------------------
 * kmalloc — Allocate from the kernel heap (first-fit free-list)
 * --------------------------------------------------------------------------- */
void *kmalloc(size_t size) {
    if (!size || !s_heap_base) return NULL;

    /* Round up to alignment and account for header + footer overhead */
    size_t aligned_size = (size + HEAP_ALIGN - 1) & ~(size_t)(HEAP_ALIGN - 1);
    size_t total_needed = aligned_size + sizeof(HeapHeader) + sizeof(HeapFooter);
    if (total_needed < HEAP_MIN_BLOCK) total_needed = HEAP_MIN_BLOCK;

    /* First-fit search through the free list */
    FreeNode *node = s_free_list;
    while (node) {
        HeapHeader *hdr = (HeapHeader *)((uint8_t *)node - sizeof(HeapHeader));

        if (hdr->magic != HEAP_MAGIC) return NULL;  /* Corruption detected */
        if (hdr->size >= total_needed) {
            free_list_remove(hdr);

            /* Split the block if remaining fragment is large enough */
            if (hdr->size >= total_needed + HEAP_MIN_BLOCK) {
                /* Create a new free block from the remainder */
                size_t remainder = hdr->size - total_needed;
                hdr->size = total_needed;

                HeapHeader *new_hdr = next_block(hdr);
                new_hdr->magic = HEAP_MAGIC;
                new_hdr->flags = 0;
                new_hdr->size  = remainder;
                block_footer(new_hdr)->size = remainder;
                free_list_insert(new_hdr);
                s_free_bytes += remainder - sizeof(HeapHeader) - sizeof(HeapFooter);
            }

            /* Mark the block as used */
            hdr->flags = 1;
            block_footer(hdr)->size = hdr->size;
            s_free_bytes -= total_needed;

            /* Return pointer to user data area */
            return (void *)((uint8_t *)hdr + sizeof(HeapHeader));
        }

        node = node->next;
    }

    return NULL; /* Out of kernel heap memory */
}

/* ---------------------------------------------------------------------------
 * kfree — Return a block to the heap with boundary-tag coalescing
 * --------------------------------------------------------------------------- */
void kfree(void *ptr) {
    if (!ptr) return;

    HeapHeader *hdr = block_header(ptr);
    if (hdr->magic != HEAP_MAGIC || !(hdr->flags & 1)) return;  /* Invalid / double-free */

    hdr->flags = 0;  /* Mark free */
    s_free_bytes += hdr->size - sizeof(HeapHeader) - sizeof(HeapFooter);
    block_footer(hdr)->size = hdr->size;

    /* Coalesce with next adjacent block if it's free */
    HeapHeader *next = next_block(hdr);
    if ((uint8_t *)next < s_heap_end && next->magic == HEAP_MAGIC && !(next->flags & 1)) {
        free_list_remove(next);
        hdr->size += next->size;
        block_footer(hdr)->size = hdr->size;
    }

    /* Coalesce with previous adjacent block if it's free */
    if ((uint8_t *)hdr > s_heap_base) {
        HeapHeader *prev = prev_block(hdr);
        if (prev->magic == HEAP_MAGIC && !(prev->flags & 1)) {
            free_list_remove(prev);
            prev->size += hdr->size;
            block_footer(prev)->size = prev->size;
            hdr = prev;
        }
    }

    free_list_insert(hdr);
}

uint64_t kheap_free_bytes(void) {
    return s_free_bytes;
}
