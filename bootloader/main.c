/**
 * @file main.c
 * @brief Freestanding UEFI x86_64 Bootloader Entry Point and Kernel Handoff
 */

#include "efi.h"
#include "gop.h"
#include "ui.h"
#include "../shared/bootinfo.h"

/* ELF64 Header definitions */
#define EI_NIDENT 16
#define ELFMAG0   0x7f
#define ELFMAG1   'E'
#define ELFMAG2   'L'
#define ELFMAG3   'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EM_X86_64 62
#define PT_LOAD   1

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

/* Global BootInfo instance */
static XenithraBootInfo g_boot_info;

/* Find ACPI RSDP table in UEFI Configuration Tables */
static uint64_t find_acpi_rsdp(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_GUID acpi2_guid = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID acpi1_guid = ACPI_10_TABLE_GUID;

    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &SystemTable->ConfigurationTable[i];

        /* Check ACPI 2.0 GUID */
        if (table->VendorGuid.Data1 == acpi2_guid.Data1 &&
            table->VendorGuid.Data2 == acpi2_guid.Data2 &&
            table->VendorGuid.Data3 == acpi2_guid.Data3) {
            return (uint64_t)table->VendorTable;
        }
    }

    /* Fallback: Check ACPI 1.0 GUID */
    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &SystemTable->ConfigurationTable[i];
        if (table->VendorGuid.Data1 == acpi1_guid.Data1 &&
            table->VendorGuid.Data2 == acpi1_guid.Data2 &&
            table->VendorGuid.Data3 == acpi1_guid.Data3) {
            return (uint64_t)table->VendorTable;
        }
    }

    return 0;
}

/* Load Kernel binary from FAT32 ESP filesystem */
static EFI_STATUS load_kernel_file(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable,
    CHAR16 *filename,
    UINT64 *out_entry_point,
    UINT64 *out_phys_base,
    UINT64 *out_size
) {
    EFI_STATUS status;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
    EFI_FILE_PROTOCOL *root_dir = NULL;
    EFI_FILE_PROTOCOL *kernel_file = NULL;

    /* Get the LoadedImageProtocol to find the device handle we booted from */
    status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &loaded_image_guid,
        (VOID**)&loaded_image
    );
    if (EFI_ERROR(status) || !loaded_image) return status;

    /* Open the filesystem volume */
    status = SystemTable->BootServices->HandleProtocol(
        loaded_image->DeviceHandle,
        &sfsp_guid,
        (VOID**)&file_system
    );
    if (EFI_ERROR(status) || !file_system) return status;

    status = file_system->OpenVolume(file_system, &root_dir);
    if (EFI_ERROR(status) || !root_dir) return status;

    /* Open kernel file */
    status = root_dir->Open(root_dir, &kernel_file, filename, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(status) || !kernel_file) {
        root_dir->Close(root_dir);
        return status;
    }

    /* Read ELF Header */
    Elf64_Ehdr ehdr;
    UINTN header_size = sizeof(Elf64_Ehdr);
    status = kernel_file->Read(kernel_file, &header_size, &ehdr);
    if (EFI_ERROR(status)) {
        kernel_file->Close(kernel_file);
        root_dir->Close(root_dir);
        return status;
    }

    /* Validate ELF magic */
    if (ehdr.e_ident[0] != ELFMAG0 || ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 || ehdr.e_ident[3] != ELFMAG3 ||
        ehdr.e_ident[4] != ELFCLASS64 || ehdr.e_machine != EM_X86_64) {
        kernel_file->Close(kernel_file);
        root_dir->Close(root_dir);
        return EFI_LOAD_ERROR;
    }

    /* Read Program Headers */
    UINTN phdr_table_size = ehdr.e_phnum * sizeof(Elf64_Phdr);
    Elf64_Phdr *phdrs = NULL;
    status = SystemTable->BootServices->AllocatePool(EfiLoaderData, phdr_table_size, (VOID**)&phdrs);
    if (EFI_ERROR(status) || !phdrs) {
        kernel_file->Close(kernel_file);
        root_dir->Close(root_dir);
        return status;
    }

    kernel_file->SetPosition(kernel_file, ehdr.e_phoff);
    status = kernel_file->Read(kernel_file, &phdr_table_size, phdrs);
    if (EFI_ERROR(status)) {
        SystemTable->BootServices->FreePool(phdrs);
        kernel_file->Close(kernel_file);
        root_dir->Close(root_dir);
        return status;
    }

    /* Load each PT_LOAD segment */
    UINT64 min_phys = 0xFFFFFFFFFFFFFFFFULL;
    UINT64 max_phys = 0;

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;

        /* Physical load target: map virtual 0xFFFFFFFF80000000 -> physical 0x200000 (2MB) or direct paddr */
        UINT64 segment_phys = phdrs[i].p_paddr;
        if (segment_phys == 0 || segment_phys == phdrs[i].p_vaddr) {
            /* Higher half kernel mapping translation */
            if (phdrs[i].p_vaddr >= 0xFFFFFFFF80000000ULL) {
                segment_phys = phdrs[i].p_vaddr - 0xFFFFFFFF80000000ULL + 0x200000ULL;
            } else {
                segment_phys = phdrs[i].p_vaddr;
            }
        }

        UINTN pages = (phdrs[i].p_memsz + 0xFFF) / 0x1000;
        EFI_PHYSICAL_ADDRESS alloc_addr = segment_phys;

        status = SystemTable->BootServices->AllocatePages(
            AllocateAddress,
            EfiLoaderData,
            pages,
            &alloc_addr
        );

        if (EFI_ERROR(status)) {
            /* Fallback: allocate any page if fixed address is taken */
            status = SystemTable->BootServices->AllocatePages(
                AllocateAnyPages,
                EfiLoaderData,
                pages,
                &alloc_addr
            );
            if (EFI_ERROR(status)) {
                SystemTable->BootServices->FreePool(phdrs);
                kernel_file->Close(kernel_file);
                root_dir->Close(root_dir);
                return status;
            }
        }

        /* Zero destination memory */
        SystemTable->BootServices->SetMem((VOID*)alloc_addr, pages * 0x1000, 0);

        /* Read segment bytes */
        if (phdrs[i].p_filesz > 0) {
            kernel_file->SetPosition(kernel_file, phdrs[i].p_offset);
            UINTN read_size = phdrs[i].p_filesz;
            kernel_file->Read(kernel_file, &read_size, (VOID*)alloc_addr);
        }

        if (alloc_addr < min_phys) min_phys = alloc_addr;
        if (alloc_addr + pages * 0x1000 > max_phys) max_phys = alloc_addr + pages * 0x1000;
    }

    *out_entry_point = ehdr.e_entry;
    *out_phys_base = min_phys;
    *out_size = (max_phys > min_phys) ? (max_phys - min_phys) : 0;

    SystemTable->BootServices->FreePool(phdrs);
    kernel_file->Close(kernel_file);
    root_dir->Close(root_dir);

    return EFI_SUCCESS;
}

/* Freestanding UEFI Application Entry Point */
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;

    /* 1. Disable UEFI Watchdog Timer */
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

    /* 2. Initialize GOP Framebuffer */
    status = gop_init(SystemTable);
    if (EFI_ERROR(status)) {
        if (SystemTable->ConOut) {
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Fatal Error: GOP Initialization Failed.\r\n");
        }
        return status;
    }

    /* 3. Run Modern Graphical Boot Menu */
    BootSelectionResult sel = ui_run_boot_menu(SystemTable, 5);

    if (sel.action == BOOT_ACTION_REBOOT) {
        SystemTable->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
        return EFI_SUCCESS;
    } else if (sel.action == BOOT_ACTION_SHUTDOWN) {
        SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
        return EFI_SUCCESS;
    }

    /* 4. Display Loading Screen */
    gop_draw_gradient_v(0, 0, g_gop_ctx.width, g_gop_ctx.height, 0x000B0E14, 0x00161B22);
    gop_draw_string_centered(g_gop_ctx.height / 2 - 30, "Starting Xenithra OS...", COLOR_TEXT_PRIMARY, 2);
    gop_draw_string_centered(g_gop_ctx.height / 2 + 10, "Initializing security subsystem & transferring control", COLOR_TEXT_SECONDARY, 1);
    gop_swap_buffers();

    /* 5. Load Kernel ELF */
    UINT64 kernel_entry = 0;
    UINT64 kernel_phys = 0;
    UINT64 kernel_size = 0;

    /* Try multiple common kernel paths on the EFI system partition */
    CHAR16 *paths[] = {
        (CHAR16*)L"\\KERNEL.ELF",
        (CHAR16*)L"\\XENITHRA\\KERNEL.ELF",
        (CHAR16*)L"\\AURAOS\\KERNEL.ELF",
        (CHAR16*)L"\\EFI\\BOOT\\KERNEL.ELF",
        NULL
    };

    status = EFI_NOT_FOUND;
    for (int p = 0; paths[p] != NULL; p++) {
        status = load_kernel_file(ImageHandle, SystemTable, paths[p], &kernel_entry, &kernel_phys, &kernel_size);
        if (!EFI_ERROR(status)) break;
    }

    if (EFI_ERROR(status)) {
        gop_fill_rect(0, g_gop_ctx.height / 2 + 50, g_gop_ctx.width, 30, COLOR_BG_DARK);
        gop_draw_string_centered(g_gop_ctx.height / 2 + 50, "Error: Could not locate KERNEL.ELF on EFI partition!", 0x00FF453A, 1);
        gop_swap_buffers();
        SystemTable->BootServices->Stall(5000000);
        return status;
    }

    /* 6. Populate Framebuffer info */
    g_boot_info.magic = XENITHRA_BOOT_MAGIC;
    g_boot_info.version = 1;
    g_boot_info.boot_mode = sel.selected_mode;
    g_boot_info.framebuffer = gop_get_framebuffer_info();
    g_boot_info.acpi_rsdp = find_acpi_rsdp(SystemTable);
    g_boot_info.kernel_phys_base = kernel_phys;
    g_boot_info.kernel_virt_base = 0xFFFFFFFF80000000ULL;
    g_boot_info.kernel_size_bytes = kernel_size;
    g_boot_info.uefi_system_table = (uint64_t)SystemTable;

    /* 7. Retrieve UEFI Memory Map */
    UINTN map_size = 0;
    EFI_MEMORY_DESCRIPTOR *mmap = NULL;
    UINTN map_key = 0;
    UINTN desc_size = 0;
    UINT32 desc_ver = 0;

    /* Get required memory map size */
    status = SystemTable->BootServices->GetMemoryMap(&map_size, mmap, &map_key, &desc_size, &desc_ver);
    map_size += 4 * desc_size; /* Add extra room for allocation */

    status = SystemTable->BootServices->AllocatePool(EfiLoaderData, map_size, (VOID**)&mmap);
    if (EFI_ERROR(status) || !mmap) {
        return status;
    }

    status = SystemTable->BootServices->GetMemoryMap(&map_size, mmap, &map_key, &desc_size, &desc_ver);
    if (EFI_ERROR(status)) {
        return status;
    }

    /* Calculate RAM statistics */
    uint64_t total_ram = 0;
    uint64_t usable_ram = 0;
    UINTN desc_count = map_size / desc_size;

    for (UINTN i = 0; i < desc_count; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)mmap + i * desc_size);
        uint64_t bytes = desc->NumberOfPages * 4096;
        total_ram += bytes;
        if (desc->Type == EfiConventionalMemory || desc->Type == EfiBootServicesCode || desc->Type == EfiBootServicesData) {
            usable_ram += bytes;
        }
    }

    g_boot_info.memory_map.map = (AuraMemoryDescriptor*)mmap;
    g_boot_info.memory_map.map_size = map_size;
    g_boot_info.memory_map.descriptor_size = desc_size;
    g_boot_info.memory_map.descriptor_version = desc_ver;
    g_boot_info.memory_map.total_memory_bytes = total_ram;
    g_boot_info.memory_map.usable_memory_bytes = usable_ram;

    /* 8. Exit Boot Services */
    status = SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
    if (EFI_ERROR(status)) {
        /* Retry once if map changed */
        map_size += 2 * desc_size;
        SystemTable->BootServices->GetMemoryMap(&map_size, mmap, &map_key, &desc_size, &desc_ver);
        status = SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    /* 9. Jump to 64-bit Kernel Entry Point (System V AMD64 ABI: RDI = BootInfo*) */
    typedef void (*KernelEntryPoint)(XenithraBootInfo *boot_info);
    KernelEntryPoint kernel_main = (KernelEntryPoint)kernel_entry;

    kernel_main(&g_boot_info);

    /* Should never return */
    while (1) {
        __asm__ volatile ("hlt");
    }

    return EFI_SUCCESS;
}
