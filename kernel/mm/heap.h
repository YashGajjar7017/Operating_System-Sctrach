/**
 * @file heap.h
 * @brief Kernel Non-Paged Pool Allocator — Boundary-Tag Free List
 */

#ifndef _KERNEL_MM_HEAP_H_
#define _KERNEL_MM_HEAP_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the kernel heap from a contiguous virtual memory region.
 * @param base  Starting virtual address of the pool (must be 16-byte aligned).
 * @param size  Total size in bytes.
 */
void kheap_init(uint64_t base, uint64_t size);

/**
 * @brief Allocate `size` bytes from the kernel non-paged pool.
 *        Returns 16-byte aligned memory. Returns NULL on failure.
 */
void *kmalloc(size_t size);

/**
 * @brief Free memory previously allocated with kmalloc().
 */
void kfree(void *ptr);

/**
 * @brief Returns total free bytes remaining in the kernel heap.
 */
uint64_t kheap_free_bytes(void);

#endif /* _KERNEL_MM_HEAP_H_ */
