/**
 * @file bootinfo.h
 * @brief ABI contract between UEFI Bootloader and the 64-bit Operating System Kernel.
 */

#ifndef _SHARED_BOOTINFO_H_
#define _SHARED_BOOTINFO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Magic signature for boot integrity verification: "AURAOS64" */
#define AURA_BOOT_MAGIC 0x3436534F41525541ULL

/* Pixel formats for GOP framebuffer */
typedef enum {
    PIXEL_RGBX_8888 = 0,
    PIXEL_BGRX_8888 = 1,
    PIXEL_BITMASK   = 2,
    PIXEL_BLT_ONLY  = 3
} AuraPixelFormat;

/**
 * @brief Linear FrameBuffer metadata passed to the kernel
 */
typedef struct {
    uint64_t base_address;       /* Physical 64-bit base address of GOP framebuffer */
    uint64_t buffer_size;        /* Total size in bytes of the linear framebuffer */
    uint32_t width;              /* Horizontal resolution in pixels */
    uint32_t height;             /* Vertical resolution in pixels */
    uint32_t pixels_per_scanline;/* Stride (pitch) in pixels per line */
    uint32_t pixel_format;       /* AuraPixelFormat */
} AuraFrameBuffer;

/**
 * @brief Memory descriptor structure representing physical RAM regions
 */
typedef struct {
    uint32_t type;               /* UEFI memory type (e.g. EfiConventionalMemory, etc.) */
    uint32_t pad;
    uint64_t physical_start;     /* Starting physical address */
    uint64_t virtual_start;      /* Starting virtual address (if mapped) */
    uint64_t number_of_pages;    /* Number of 4KB pages */
    uint64_t attribute;          /* Memory attributes / caching flags */
} AuraMemoryDescriptor;

/**
 * @brief UEFI Memory Map information for kernel Physical Memory Manager (PMM)
 */
typedef struct {
    AuraMemoryDescriptor* map;   /* Array of memory descriptors */
    uint64_t map_size;           /* Total size in bytes of descriptor table */
    uint64_t descriptor_size;    /* Size in bytes of each descriptor */
    uint64_t descriptor_version; /* Descriptor version */
    uint64_t total_memory_bytes; /* Total detected physical RAM */
    uint64_t usable_memory_bytes;/* Total usable conventional RAM */
} AuraMemoryMap;

/**
 * @brief Complete Boot Information structure passed via RDI to Kernel Entry
 */
typedef struct {
    uint64_t magic;              /* Must equal AURA_BOOT_MAGIC */
    uint32_t version;            /* Bootloader protocol version */
    uint32_t boot_mode;          /* 0 = Normal Desktop, 1 = Safe/Debug Mode, 2 = Diagnostics */
    
    AuraFrameBuffer framebuffer; /* GOP FrameBuffer configuration */
    AuraMemoryMap   memory_map;  /* Complete physical memory map */
    
    uint64_t acpi_rsdp;          /* Physical address of ACPI 1.0/2.0 RSDP table */
    uint64_t kernel_phys_base;   /* Physical base address where kernel binary was loaded */
    uint64_t kernel_virt_base;   /* Higher-half virtual base address of kernel */
    uint64_t kernel_size_bytes;  /* Size of kernel binary in bytes */
    
    uint64_t uefi_system_table;  /* Pointer to UEFI System Table (before ExitBootServices) */
} AuraBootInfo;

#ifdef __cplusplus
}
#endif

#endif /* _SHARED_BOOTINFO_H_ */
