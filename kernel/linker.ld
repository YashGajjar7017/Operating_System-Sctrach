/*
 * @file linker.ld
 * @brief Linker script for 64-bit higher-half OS kernel
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

KERNEL_VMA = 0xFFFFFFFF80000000;
KERNEL_LMA = 0x0000000000200000;

SECTIONS
{
    . = KERNEL_VMA + KERNEL_LMA;

    __kernel_start = .;

    /* Kernel Code Section */
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        __text_start = .;
        *(.text._start)
        *(.text .text.*)
        __text_end = .;
    }

    /* Read-Only Data (Constants, Strings, Fonts) */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        __rodata_start = .;
        *(.rodata .rodata.*)
        __rodata_end = .;
    }

    /* Initialized Read-Write Data */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    }

    /* Uninitialized Read-Write Data & BSS */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
    }

    . = ALIGN(4K);
    __kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
