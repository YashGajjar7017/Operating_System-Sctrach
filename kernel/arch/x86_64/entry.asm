; -----------------------------------------------------------------------------
; @file entry.asm
; @brief 64-bit Kernel Entry Point (System V AMD64 ABI)
; -----------------------------------------------------------------------------

[BITS 64]

global _start
extern kmain

section .text._start
_start:
    ; Disable interrupts immediately upon entering kernel space
    cli

    ; Clear direction flag for string operations
    cld

    ; Setup initial 64-bit kernel execution stack (16-byte aligned)
    mov rsp, kernel_stack_top
    and rsp, -16

    ; Pass BootInfo pointer (passed in RDI from bootloader) to kmain
    ; In System V AMD64 ABI: 1st argument = RDI
    call kmain

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
kernel_stack_bottom:
    resb 65536 ; 64 KB Initial Kernel Stack
kernel_stack_top:
