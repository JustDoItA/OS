/*
 * @lpf
 * Mon Oct 23 07:20:07 PDT 2017
 */
.globl page_fault

page_fault:
    xchgl %eax,(%esp)
    pushl %ecx
    pushl %edx
    push %ds
    push %es
    push %fs
    movl $0x10,%edx
    mov %dx,%es
    mov %dx,%fs
    movl %cr2,%edx
    pushl %edx
    pushl %eax
    testl $1,%eax
    jne 1f
    call do_no_page
    jmp 2f
1:  call do_wp_page
2:  addl $8,%esp
    pop %fs
    pop %es
    pop %ds
    popl %edx
    popl %ecx
    popl %eax
    iret
