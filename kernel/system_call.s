/*
 *linux/kernel/system_call.s
 * Fri Aug 25 19:54:07 PDT 2017
*/

OLDSS   = 0x2c

signal  = 12

blocked = (33*16)

sa_handler = 0

sa_restorer = 12

.global sys_fork
.global parallel_interrupt
.global device_not_available, coprocessor_error

.align 4
coprocessor_error:
    push %ds
    push %es
    push %fs
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    mov $0x10,%eax
    mov %ax,%ds
    mov %ax,%es
    movl $0x17,%eax
    mov %ax,%fs
    pushl $ret_from_sys_call
    jmp math_error

.align 4
device_not_available:
    push %ds
    push %es
    push %fs
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    movl $0x10,%eax
    mov %ax,%ds
    mov %ax,%es
    mov $0x17,%eax
    mov %ax,%fs
    pushl $ret_from_sys_call
    clts
    movl %cr0,%eax
    je math_state_restore
    pushl %ebp
    pushl %esi
    pushl %edi
    call math_emulate
    popl %edi
    popl %esi
    popl %ebp
    ret

ret_from_sys_call:
    movl current,%eax
    cmpl task,%eax
    je 3f
    cmpw $0x17,OLDSS(%esp)
    jne 3f
    movl signal(%eax),%ebx
    movl blocked(%eax),%ecx
    notl %ecx
    andl %ebx,%ecx
    bsfl %ecx,%ecx
    je 3f
    btrl %ecx,%ebx
    movl %ebx,signal(%eax)
    incl %ecx
    pushl %ecx
    call do_signal
    popl %eax
3:  popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    pop %fs
    pop %es
    pop %ds
    iret
parallel_interrupt:
    pushl %eax
    movb $0x20,%al
    outb %al,$0x20
    popl %eax
    iret

.align 4
sys_fork:
    call find_empty_process
    testl %eax, %eax
    js 1f
    push %gs
    pushl %esi
    pushl %edi
    pushl %ebp
    pushl %eax
    call copy_process
    addl $20, %esp
1:  ret
