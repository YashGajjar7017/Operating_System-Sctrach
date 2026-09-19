; =============================================================================
; @file gdt_idt.asm
; @brief GDT/IDT Load Stubs, ISR Entry Templates, and TSS Load for Xenithra OS
;
; Three distinct sections:
;   1. GDT/IDT/TSS loaders  — _gdt_load_asm, _idt_load_asm, _tss_load_asm
;   2. ISR stub generator   — 256 unified entry stubs via NASM macro
;   3. isr_stub_table[]     — array of 256 function pointers for C IDT setup
; =============================================================================

[BITS 64]

global _gdt_load_asm
global _idt_load_asm
global _tss_load_asm
global isr_stub_table
global syscall_init_msrs

extern isr_common_handler      ; C handler: void isr_common_handler(InterruptFrame *frame)

section .text

; -----------------------------------------------------------------------------
; _gdt_load_asm(DT_REGISTER *gdtr)  [System V AMD64: RDI = gdtr pointer]
;
; Loads the GDT via LGDT, then performs a far-return (retfq) to reload CS
; with the new Kernel Code Segment selector (0x08), flushing the instruction
; prefetch queue and pipeline decode state.
;
; After LGDT all data segment registers must be reloaded to pick up new bases.
; SS must be reloaded with the Kernel Data selector (0x10).
; DS, ES, FS, GS are set to 0 (null) — segment registers are unused in
; 64-bit mode for flat addressing, except FS/GS for TLS/per-CPU KPCR.
; -----------------------------------------------------------------------------
_gdt_load_asm:
    lgdt [rdi]                  ; Load GDTR from DT_REGISTER struct at RDI

    ; Reload all data segments
    mov ax, 0x10                ; SEL_KERNEL_DATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far-return to reload CS: push new CS (0x08) then new RIP, then retfq
    ; This is the canonical way to do a 64-bit far jump without JMP FAR
    lea rax, [rel .flush_cs]    ; RIP target for the far-return
    push 0x08                   ; Push new Code Segment selector
    push rax                    ; Push new RIP
    retfq                       ; Far return: pops RIP then CS

.flush_cs:
    ret

; -----------------------------------------------------------------------------
; _idt_load_asm(DT_REGISTER *idtr)  [System V AMD64: RDI = idtr pointer]
; -----------------------------------------------------------------------------
_idt_load_asm:
    lidt [rdi]
    ret

; -----------------------------------------------------------------------------
; _tss_load_asm(uint16_t selector)  [System V AMD64: DI = selector]
; The LTR instruction marks the TSS as busy in the GDT. Only valid after LGDT.
; -----------------------------------------------------------------------------
_tss_load_asm:
    ltr di
    ret

; -----------------------------------------------------------------------------
; syscall_init_msrs — Configure SYSCALL/SYSRET MSRs for fast system calls
;
; IA32_STAR   (0xC0000081): Segment selectors for SYSCALL/SYSRET
;   Bits [47:32] = Kernel CS for SYSCALL entry (must be SEL_KERNEL_CODE = 0x08)
;                  Kernel SS = STAR[47:32] + 8   → 0x10
;   Bits [63:48] = User CS for SYSRET            (must be SEL_USER_DATA = 0x18)
;                  User  CS = STAR[63:48] + 16   → 0x28 - but SYSRET returns to user
;                             Actually: STAR[63:48]+0 = SS, STAR[63:48]+8 = CS on SYSRET
;
; IA32_LSTAR  (0xC0000082): 64-bit SYSCALL entry point RIP
; IA32_FMASK  (0xC0000084): RFLAGS bits to clear on SYSCALL entry
;   Clear IF (bit 9): disable interrupts during syscall dispatch
;   Clear DF (bit 10): clear direction flag (ABI requires this)
; -----------------------------------------------------------------------------
syscall_init_msrs:
    ; Load STAR MSR: [63:48]=User base (0x18), [47:32]=Kernel base (0x08)
    mov ecx, 0xC0000081             ; MSR_IA32_STAR
    xor eax, eax
    mov edx, 0x00180008             ; High DWORD: [31:16]=User(0x18), [15:0]=Kernel(0x08)
    wrmsr

    ; Load LSTAR MSR: 64-bit syscall entry point
    mov ecx, 0xC0000082             ; MSR_IA32_LSTAR
    lea rax, [rel syscall_entry]    ; Entry point defined in syscall.asm
    mov rdx, rax
    shr rdx, 32                     ; EDX = high 32 bits, EAX = low 32 bits
    wrmsr

    ; Load FMASK MSR: clear IF and DF on SYSCALL
    mov ecx, 0xC0000084             ; MSR_IA32_FMASK
    mov eax, (1 << 9) | (1 << 10)  ; IF | DF
    xor edx, edx
    wrmsr

    ; Enable SYSCALL/SYSRET in IA32_EFER (Bit 0 = SCE)
    mov ecx, 0xC0000080             ; MSR_IA32_EFER
    rdmsr
    or eax, 1                       ; Set SCE (SysCall Enable)
    wrmsr

    ret

; =============================================================================
; ISR Entry Stub Generator
;
; Each of the 256 IDT vectors gets a unique stub that:
;   1. Optionally pushes a dummy error code (for vectors that don't push one)
;   2. Pushes the vector number
;   3. Jumps to the unified isr_common_entry handler
;
; CPU automatically pushes onto the stack (high to low address):
;   SS, RSP, RFLAGS, CS, RIP   [+optional error code for some vectors]
;
; Vectors that push an error code automatically: 8, 10-14, 17, 21, 29, 30
; =============================================================================

; Macro for exceptions WITHOUT an automatic hardware error code
%macro ISR_NOERRCODE 1
isr_stub_%1:
    push qword 0            ; Dummy error code to align the frame
    push qword %1           ; Vector number
    jmp isr_common_entry
%endmacro

; Macro for exceptions WITH an automatic hardware error code
%macro ISR_ERRCODE 1
isr_stub_%1:
    ; Error code already on stack — CPU pushed it
    push qword %1           ; Vector number (after the error code)
    jmp isr_common_entry
%endmacro

; Generate all 256 stubs
ISR_NOERRCODE 0    ; #DE Divide Error
ISR_NOERRCODE 1    ; #DB Debug Exception
ISR_NOERRCODE 2    ; NMI
ISR_NOERRCODE 3    ; #BP Breakpoint
ISR_NOERRCODE 4    ; #OF Overflow
ISR_NOERRCODE 5    ; #BR BOUND Range Exceeded
ISR_NOERRCODE 6    ; #UD Invalid Opcode
ISR_NOERRCODE 7    ; #NM Device Not Available
ISR_ERRCODE   8    ; #DF Double Fault (error code = 0)
ISR_NOERRCODE 9    ; Coprocessor Segment Overrun (legacy)
ISR_ERRCODE   10   ; #TS Invalid TSS
ISR_ERRCODE   11   ; #NP Segment Not Present
ISR_ERRCODE   12   ; #SS Stack-Segment Fault
ISR_ERRCODE   13   ; #GP General Protection Fault
ISR_ERRCODE   14   ; #PF Page Fault
ISR_NOERRCODE 15   ; Reserved
ISR_NOERRCODE 16   ; #MF x87 FPU Floating-Point Error
ISR_ERRCODE   17   ; #AC Alignment Check
ISR_NOERRCODE 18   ; #MC Machine Check
ISR_NOERRCODE 19   ; #XM/#XF SIMD Floating-Point Exception
ISR_NOERRCODE 20   ; #VE Virtualization Exception
ISR_ERRCODE   21   ; #CP Control Protection Exception
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29   ; #HV Hypervisor Injection Exception
ISR_ERRCODE   30   ; #VC VMM Communication Exception
ISR_NOERRCODE 31   ; #SX Security Exception

; IRQ vectors 32-255 (hardware and software interrupts)
%assign i 32
%rep 224
    ISR_NOERRCODE i
%assign i i+1
%endrep

; =============================================================================
; isr_common_entry — Unified exception/interrupt handler entry point
;
; Stack layout on entry (grows DOWN, addresses decrease):
;   [RSP+0]  = vector number (pushed by stub)
;   [RSP+8]  = error code or dummy 0 (pushed by stub or CPU)
;   [RSP+16] = RIP    (CPU-pushed)
;   [RSP+24] = CS     (CPU-pushed)
;   [RSP+32] = RFLAGS (CPU-pushed)
;   [RSP+40] = RSP    (CPU-pushed, present only on privilege change)
;   [RSP+48] = SS     (CPU-pushed, present only on privilege change)
;
; We save ALL general-purpose registers to build a complete InterruptFrame,
; then call the C handler. On return we restore and IRETQ back.
; =============================================================================
isr_common_entry:
    ; Save all general-purpose registers (preserving error code + vector already on stack)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Swap GS to get kernel KPCR (if coming from userland)
    ; swapgs is needed when CS on entry was user-mode (DPL=3)
    mov rax, [rsp + (15*8 + 8 + 8 + 8)]  ; Load saved CS from CPU-pushed frame
    and rax, 3                             ; Extract RPL bits
    jz .skip_swapgs_entry                  ; If RPL=0 (kernel), no swap needed
    swapgs
.skip_swapgs_entry:

    ; Pass pointer to the full InterruptFrame to the C handler
    ; RDI = pointer to bottom of saved registers (= current RSP)
    mov rdi, rsp
    call isr_common_handler                ; Call C dispatch routine

    ; Restore GS if we swapped it on entry
    mov rax, [rsp + (15*8 + 8 + 8 + 8)]
    and rax, 3
    jz .skip_swapgs_exit
    swapgs
.skip_swapgs_exit:

    ; Restore all general-purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Pop vector number and error code (pushed by stub)
    add rsp, 16

    ; Return from interrupt (restores RIP, CS, RFLAGS, RSP, SS from CPU frame)
    iretq

; Placeholder for syscall entry (implemented in syscall.asm)
global syscall_entry
syscall_entry:
    ; Implementation is in kernel/arch/x86_64/syscall.asm
    ; This label must exist here for the LSTAR MSR reference
    ; but the real body is in syscall.asm via global declaration
    ret

; =============================================================================
; isr_stub_table — Array of 256 ISR stub addresses for C IDT initialization
; =============================================================================
section .data
align 8
isr_stub_table:
%assign j 0
%rep 256
    dq isr_stub_%+j
%assign j j+1
%endrep
