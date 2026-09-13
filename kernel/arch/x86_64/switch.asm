; =============================================================================
; Xenithra OS x86_64 Hardware Context Switcher
; =============================================================================

[BITS 64]
global switch_to_thread

section .text

; void switch_to_thread(Thread *prev, Thread *next)
; System V AMD64 ABI: RDI = prev, RSI = next
switch_to_thread:
    ; 1. If prev thread is NULL, jump straight to loading next thread
    test rdi, rdi
    jz .load_next

    ; 2. Save Callee-Saved Registers onto current stack
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    pushfq

    ; 3. Save FPU / SSE State into prev->fpu_state (Offset: 48 bytes into Thread)
    lea rax, [rdi + 48]
    fxsave64 [rax]

    ; 4. Save current stack pointer into prev->rsp (Offset: 32 bytes into Thread)
    mov [rdi + 32], rsp

.load_next:
    ; 5. Load stack pointer from next->rsp
    mov rsp, [rsi + 32]

    ; 6. Switch Virtual Address Space (CR3) if cr3 is non-zero and differs
    mov rax, [rsi + 40]             ; next->cr3
    test rax, rax
    jz .restore_fpu
    mov rcx, cr3
    cmp rax, rcx
    je .restore_fpu
    mov cr3, rax

.restore_fpu:
    ; 7. Restore FPU / SSE State from next->fpu_state
    lea rax, [rsi + 48]
    fxrstor64 [rax]

    ; 8. Restore Callee-Saved Registers from new stack
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; 9. Return to the instruction pointer stored on next thread's stack
    ret
