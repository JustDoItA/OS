/*
  @lpf
  Sat Nov 11 06:45:12 PST 2017
*/

.text
.globl keyboard_interrupt

keyboard_interrupt:
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    push  %ds
    push  %es
    movl $0x10, %eax
    mov %ax, %ds
    mov %ax, %es
    xor %al, %al
    inb $0x60, %al
    cmpb $0xe0, %al
    #je set_e0
    #cmpb $e1, %al
    #je set_e1
