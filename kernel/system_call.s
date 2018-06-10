/*
 *linux/kernel/system_call.s
 * Fri Aug 25 19:54:07 PDT 2017
*/

CS = 0x20

OLDSS   = 0x2c

state = 0
counter = 4


signal  = 12

blocked = (33*16)

sa_handler = 0

sa_restorer = 12

nr_system_calls = 72

.global system_call,sys_fork,timer_interrupt
.global hd_interrupt,floppy_interrupt,parallel_interrupt
.global device_not_available, coprocessor_error

.align 4
bad_sys_call:
    movl $-1, %eax
    iret

.align 4
reschedule:
    pushl $ret_from_sys_call
    jmp schedule

.align 4
system_call:
    cmpl $nr_system_calls-1, %eax
    ja bad_sys_call
    push %ds
    push %es
    push %fs
    pushl %edx
    pushl %ecx
    pushl %ebx
    movl  $0x10, %edx
    mov %dx,%ds
    mov %dx,%es
    movl $0x17, %edx
    mov %dx, %fs
    call *sys_call_table(,%eax,4)
    #call sys_call_table(,%eax,4)
    pushl %eax
    movl current,%eax
    cmpl $0,state(%eax)
    jne reschedule
    cmpl $0,counter(%eax)
    je reschedule
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

.align 4
timer_interrupt:
    push %ds
    push %es
    push %fs
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    movl $0x10, %eax
    mov %ax, %ds
    mov %ax, %es
    movl $0x17,%eax
    mov %ax, %fs
    incl jiffies
    movb $0x20, %al
    outb %al, $0x20
    movl CS(%esp), %eax
    andl $3, %eax
    pushl %eax
    call do_timer
    addl $4,%esp
    jmp ret_from_sys_call

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

hd_interrupt:
    pushl %eax
    pushl %ecx
    pushl %edx
    push %ds
    push %es
    push %fs
    movl $0x10,%eax
    mov %ax, %ds
    mov %ax, %es
    movl $0x17,%eax
    mov %ax,%fs
    movb $0x20, %al
    outb %al, $0xA0
    jmp 1f
1:  jmp 1f
1:  xorl %edx,%edx
    xchgl do_hd,%edx
    testl %edx,%edx
    jne 1f
    movl $unexpected_hd_interrupt,%edx
1:  outb %al,$0x20
    call *%edx
    pop %fs
    pop %es
    pop %ds
    popl %edx
    popl %ecx
    popl %eax
    iret

floppy_interrupt:
    pushl %eax
    pushl %ecx
    pushl %edx
    push %ds
    push %es
    push %fs
    movl $0x10,%eax
    mov %ax, %ds
    mov %ax, %es
    movl $0x17, %eax
    mov %ax, %fs
    movb $0x20, %al
    outb %al, $0x20
    xorl %eax, %eax
    xchgl do_floppy, %eax
    testl %eax, %eax
    jne 1f
    movl $unexpected_floppy_interrupt, %eax
1:  call *%eax
    pop %fs
    pop %es
    pop %ds
    popl %edx
    popl %ecx
    popl %eax
    iret
