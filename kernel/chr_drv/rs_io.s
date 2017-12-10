/*
 @lpf
 Sun Nov  5 04:12:45 PST 2017
*/

.text
.globl rs1_interrupt, rs2_interrupt

size = 1024

rs_addr = 0
head = 4
tail = 8
proc_list = 12
buf = 16

startup = 256

.align 4
rs1_interrupt:
    pushl $table_list + 8
    jmp rs_int
.align 4
rs2_interrupt:
    pushl $table_list + 16
rs_int:
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    push %es
    push %ds
    pushl $0x10
    pop %ds
    pushl $0x10
    pop %es
    movl 24(%esp), %edx
    movl (%edx), %edx
    movl rs_addr(%edx), %edx
    addl $2, %edx
rep_int:
    xorl %eax, %eax
    inb %dx, %al
    testb $1, %al
    jne end
    cmpb $6, %al
    ja end
    movl 24(%esp), %ecx
    pushl %edx
    subl $2, %edx
    call *jmp_table(,%eax,2)
    popl %edx
    jmp rep_int
end:
    movb $0x20, %al
    outb %al, $0x20
    pop %ds
    pop %es
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    addl $4, %esp
    iret

jmp_table:
    .long modem_status,write_char,read_char,line_status


.align 4
modem_status:
    addl $6, %edx
    inb %dx, %al
    ret

.align 4
line_status:
    addl $5, %edx
    inb %dx, %al
    ret

.align 4
read_char:
    inb %dx, %al
    movl %ecx, %edx
    subl $table_list, %edx
    shrl $3, %edx
    movl (%ecx), %ecx
    movl head(%ecx), %ebx
    incl %ebx
    andl $size -1, %ebx
    cmpl tail(%ecx), %ebx
    je 1f
    mov %ebx, head(%ecx)
1:  pushl %edx
    call do_tty_interrupt
    addl $4, %esp
    ret

.align 4
write_char:
    movl 4(%ecx), %ecx
    movl head(%ecx), %ebx
    subl tail(%ecx),%ebx
    addl $size -1, %ebx
    je write_buffer_empty
    cmpl $startup, %ebx
    ja 1f
    movl proc_list(%ecx), %ebx
    testl %ebx, %ebx
    je 1f
    movl $0, (%ebx)
1:  movl tail(%ecx), %ebx
    movb buf(%ecx,%ebx), %al
    outb %al, %dx
    incl %ebx
    andl $size - 1, %ebx
    movl %ebx, tail(%ecx)
    cmpl head(%ecx), %ebx
    je write_buffer_empty
    ret
.align 4
write_buffer_empty:
    movl proc_list(%ecx), %ebx
    testl %ebx, %ebx
    je 1f
    movl $0, (%ebx)
1:  incl %edx
    inb %dx, %al
    jmp 1f
1:  jmp 1f
1:  andb $0xd, %al
    outb %al, %dx
    ret
