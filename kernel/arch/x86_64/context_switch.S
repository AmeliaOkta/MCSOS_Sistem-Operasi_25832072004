    .section .text
    .globl mcsos_context_switch
    .type  mcsos_context_switch, @function
mcsos_context_switch:
    leaq 1f(%rip), %rax
    movq %rsp,  0(%rdi)
    movq %rbp,  8(%rdi)
    movq %rbx, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)
    movq %rax, 56(%rdi)
    movq  0(%rsi), %rsp
    movq  8(%rsi), %rbp
    movq 16(%rsi), %rbx
    movq 24(%rsi), %r12
    movq 32(%rsi), %r13
    movq 40(%rsi), %r14
    movq 48(%rsi), %r15
    jmp *56(%rsi)
1:
    ret
    .size mcsos_context_switch, . - mcsos_context_switch
