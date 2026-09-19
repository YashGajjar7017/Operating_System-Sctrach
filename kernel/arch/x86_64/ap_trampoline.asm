; =============================================================================
; @file ap_trampoline.asm
; @brief Application Processor (AP) SIPI Wakeup Trampoline — Xenithra OS
;
; This code is copied at runtime to physical address 0x8000 by the BSP before
; sending the SIPI (Startup IPI). The SIPI vector = 0x08 encodes this address
; (0x08 × 4096 = 0x8000). All APs start executing here in 16-bit Real Mode.
;
; Transition Path (matching the BSP's boot sequence from the bootloader):
;   16-bit Real Mode  → Load early flat GDT, set CR0.PE=1, far-jump
;   32-bit Protected  → Enable PAE (CR4.PAE=1), enable Long Mode (EFER.LME=1)
;   64-bit Long Mode  → Load CR3 from BSP-written location, set CR0.PG=1, far-jump
;   ap_kernel_entry_c → Per-AP KPCR init, join idle loop
;
; Fixed Memory Map (written by BSP before SIPI):
;   0x7000 : AP Kernel Entry C function pointer (8 bytes)
;   0x7008 : BSP PML4 physical address (8 bytes)
;   0x7010 : Early AP GDT (16 bytes: null + code + data)
;   0x7020 : Early AP GDTR (10 bytes: limit + base)
;   0x702A : AP barrier counter (4 bytes) — BSP increments to signal ready
; =============================================================================

global ap_trampoline_start
global ap_trampoline_end

section .text

[BITS 16]
ap_trampoline_start:

; =============================================================================
; Stage 1: 16-bit Real Mode Setup
; =============================================================================
    cli                          ; Disable interrupts immediately
    cld                          ; Clear direction flag

    ; Establish a real-mode data segment at CS
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00               ; Temporary real-mode stack below our code

    ; Load early AP GDT (stored at physical 0x7010)
    lgdt [0x7020]                ; Load GDTR: limit=23, base=0x7010

    ; Enable 32-bit Protected Mode: set CR0.PE (bit 0)
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax

    ; Far-jump to flush pipeline and reload CS with flat 32-bit code selector (0x08)
    jmp dword 0x08:(0x8000 + (ap_32bit_entry - ap_trampoline_start))

; =============================================================================
; Stage 2: 32-bit Protected Mode — Enable Paging for Long Mode
; =============================================================================
[BITS 32]
ap_32bit_entry:
    ; Reload data segments with flat 32-bit data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Temporary 32-bit stack (1KB below trampoline)
    mov esp, 0x7BF0

    ; Enable PAE (Physical Address Extension) — required for Long Mode
    mov eax, cr4
    or eax, (1 << 5)            ; CR4.PAE = 1
    mov cr4, eax

    ; Load BSP's PML4 physical address (stored at 0x7008) into CR3
    mov eax, [0x7008]
    mov cr3, eax

    ; Enable Long Mode in IA32_EFER MSR (bit 8 = LME)
    mov ecx, 0xC0000080         ; IA32_EFER MSR index
    rdmsr
    or eax, (1 << 8)            ; Set LME (Long Mode Enable)
    wrmsr

    ; Enable Paging (CR0.PG = bit 31) to activate Long Mode
    mov eax, cr0
    or eax, (1 << 31)           ; CR0.PG = 1
    mov cr0, eax

    ; Far-jump to flush pipeline and enter 64-bit mode with kernel CS (0x08)
    jmp dword 0x08:(0x8000 + (ap_64bit_entry - ap_trampoline_start))

; =============================================================================
; Stage 3: 64-bit Long Mode — Call into C AP initialization
; =============================================================================
[BITS 64]
ap_64bit_entry:
    ; Reload all data segments to 64-bit kernel data selector
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax

    ; Set up a minimal 64-bit stack (AP gets its own stack from kpcr_init)
    mov rsp, 0x7BF0

    ; Clear the frame pointer to mark the bottom of the call stack for backtraces
    xor rbp, rbp

    ; Read the ap_kernel_entry_c function pointer from 0x7000
    mov rax, [0x7000]
    test rax, rax
    jz ap_hang              ; Null pointer: AP goes idle

    ; Call into C: void ap_kernel_entry_c(void)
    call rax

ap_hang:
    cli
    hlt
    jmp ap_hang

ap_trampoline_end:

; =============================================================================
; Early AP GDT — lives at fixed physical address 0x7010
; =============================================================================
section .rodata
align 8
ap_early_gdt:
    ; Descriptor 0: Null
    dq 0x0000000000000000
    ; Descriptor 1 (0x08): 32-bit Flat Code  — Present, DPL=0, Code, Exec/Read, G=4KB, D=32bit
    dq 0x00CF9A000000FFFF
    ; Descriptor 2 (0x10): 32-bit Flat Data  — Present, DPL=0, Data, Read/Write, G=4KB, B=32bit
    dq 0x00CF92000000FFFF

ap_early_gdtr:
    dw 23           ; Limit = 3 entries × 8 bytes - 1
    dq ap_early_gdt ; Base
