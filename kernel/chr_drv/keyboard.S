/*
  @lpf
  Sat Nov 11 06:45:12 PST 2017
*/
#include <linux/config.h>

.text
.globl keyboard_interrupt

//size 键盘缓冲区的长度
size = 1024

//以下这些是缓冲队列结构中的偏移量
head = 4 /*缓冲区中头指针字段偏移*/
tail = 8 /*缓冲区中尾指针字段偏移*/
proc_list = 12 /*等待该缓冲队列的进程字段偏移*/
buf = 16        /*缓冲区字段偏移*/

//mode 是键盘特殊键按下状态标志
// 表示大小写转换键caps，交换键alt，控制键ctrl和换挡键shift
// 位7 caps键按下
// 位6 caps键状态（应该与leds中对应标志位一样）
// 位5 右alt键按下
// 位4 左alt键按下
// 位3 右ctrl键按下
// 位2 左ctrl键按下
// 位1 右shift键按下
// 位0 左shift键按下
mode: .byte 0

// 数字锁定键num-lock，大小写转换键caps-lock和滚动锁定键
//scroll-lock的LED发光管状态
//位7-3全0不用
//位2 caps-lock
//位1 num-lock 初始置1，即设置数字锁定键num-lock发光管位亮
//位0 scroll-lock
leds: .byte 2

//当扫描码是0xe0 或 0xe1时，置该标志。表示其后还跟随者1个或
//两个字符扫描码
//位1=1 收到0xe1标志
//位0=1 收到0xe0标志

e0:  .byte 0

keyboard_interrupt:
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    push  %ds
    push  %es
    movl $0x10, %eax  /*将ds，es段寄存器置位内核数据段*/
    mov %ax, %ds
    mov %ax, %es
    xor %al, %al
    inb $0x60, %al    /*读取扫描码到al寄存器*/
    cmpb $0xe0, %al     /*如果扫描码是0xe0*/
    je set_e0
    cmpb $0xe0, %al
    je set_e1
    call *key_table(,%eax,4) /*调用键盘处理程序key_table + eax *4*/
    movb $0, e0             /*复位e0标志位*/

//下面代码是针对8255A的PC标准键盘电路进行硬件复位处理，端口0x61
//是8259A输出口B的地址，该输出端口的第7位用于禁止和允许对键盘数据的处理
//这段程序用于对收到的扫描码做出应答。方法是首先禁止键盘，然后立即重新允许
//键盘中断
e0_e1:   inb $0x61, %al  /*取PPI端口B状态，其位7用于允许/禁止（0/1）键盘*/
    jmp 1f
1:  jmp 1f
1:  orb $0x80, %al
    jmp 1f
1:  jmp 1f
1:  outb %al, $0x61
    jmp 1f
1:  jmp 1f
1:  andb $0x7F, %al
    outb %al,$0x61
    movb $0x20, %al
    outb %al, $0x20
    pushl $0
    call do_tty_interrupt
    addl $4, %esp
    pop %es
    pop %ds
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    iret

set_e0:  movb $1, e0
    jmp e0_e1
set_e1:  movb $2, e0
    jmp e0_e1


put_queue:
    pushl %ecx          /*保持ecx，edx内容*/
    pushl %edx
    movl table_list,%edx
    movl head(%edx), %ecx  /*取缓冲队列中的头指针*/
1:  movb %al,buf(%edx,%ecx) /*将al中的字符放入缓冲队列头指针位置处*/
    incl %ecx               /*头指针前移1字节*/
    andl $size-1, %ecx      /*已缓冲区大小调整指针(若超出则返回缓冲区开始)*/
    /*头指针=尾指针吗？*/
    cmpl tail(%edx),%ecx
    je 3f                   /*如果队列已满，则后面未放入的字符全部抛弃*/
    shrdl $8,%ebx,%eax     /*将ebx中8位比特位右移8位到eax中，但ebx不变*/
    je 2f                   /*还有字符码？若没有（等于0）则跳转*/
    shrl $8,%ebx            /*将ebx中比特位右移8位，并跳转到标号1继续操作*/
    jmp 1b
2:  movl %ecx,head(%edx)     /*若已将所有字符都放入了队列，则保存头指针*/
    movl proc_list(%edx),%ecx /*该队列的等待进程指针*/
    testl %ecx, %ecx            /*检测任务结构指针是否位空（有等待该队列的进程吗？）*/
    je 3f                   /*无，则跳转*/
    movl $0,(%ecx)          /*有则置该进程为可运行就绪状态（唤醒该进程）*/
3:  popl %edx               /*弹出保留的寄存器并返回*/
    popl %ecx
    ret

ctrl:   movb $0x04, %al
    jmp 1f
alt:    movb $0x10, %al
1:  cmpb $0,e0
    je 2f
    addb %al,%al
2:  orb %al,mode
    ret
unctrl: movb $0x04,%al
    jmp 1f
unalt:  movb $0x10,%al
1:  cmpb $0,e0
    je 2f
    addb %al,%al
2:  notb %al
    andb %al,mode
    ret

lshift:
    orb $0x01, mode
    ret
unlshift:
    andb $0xfe, mode
    ret
rshift:
    orb $0x02, mode
    ret
unrshift:
    andb $0xfd, mode
    ret

caps:
    testb $0x80, mode
    jne 1f
    xorb $4, leds
    xorb $0x40, mode
    orb $0x80, mode
set_leds:
    call kb_wait
    movb $0xed, %al
    outb %al, $0x60
    call kb_wait
    movb leds, %al
    outb %al, $0x60
    ret
uncaps:
    andb $0x7f, mode
    ret
scroll:
    xorb $1, leds
    jmp set_leds
num:
    xorb $2, leds
    jmp set_leds


cursor:
    subb $0x47,%al
    jb 1f
    cmpb $12, %al
    ja 1f
    jne cur2
    testb $0x30,mode
    jne reboot
cur2:
    cmpb $0x01, e0
    je cur
    testb $0x03, mode
    jne cur
    xorl %ebx, %ebx
    movb num_table(%eax), %al
    jmp put_queue
1:  ret

cur:
    movb cur_table(%eax), %al
    cmpb $'9,%al
    ja ok_cur
    movb $'~, %al
ok_cur:
    shll $16, %eax
    movw $0x5b1b, %ax
    xorl %ebx, %ebx
    jmp put_queue

#if defined(KBD_FR)
num_table:
    .ascii "789 456 1230."
#else
num_table:
    .ascii "789 456 1230,"
#endif
cur_table:
    .ascii "HA5 DGC YB623"


func:
    pushl %eax
    pushl %ecx
    pushl %edx
    call show_stat
    popl %edx
    popl %ecx
    popl %eax
    subb $18,%al
    jb    end_func
    cmpb $9, %al
    jb end_func
    cmpb $11,%al
    ja end_func
ok_func:
    cmpl $4, %ecx
    jl end_func
    movl func_table(,%eax,4),%eax
    xorl %ebx, %ebx
    jmp put_queue
end_func:
    ret


func_table:
    .long 0x415b5b1b,0x425b5b1b,0x435b5b1b,0x445b5b1b
    .long 0x455b5b1b,0x465b5b1b,0x475b5b1b,0x485b5b1b
    .long 0x495b5b1b,0x4a5b5b1b,0x4b5b5b1b,0x4c5b5b1b

#if defined(KBD_FINNISH)
key_map:
    .byte 0,27
    .ascii "1234567890+"
    .byte 127,9
    .ascii "qwertyuiop}"
    .byte 0,13,0
    .ascii "asdfghjkl|{"
    .byte 0,0
    .ascii "'zxcvbnm,.-"
    .byte 0,'*,0,32
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '<
    .fill 10,1,0

shift_map:
    .byte 0,27
    .ascii "!\"#$%&/()=?`"
    .byte 127,9
    .ascii "QWERTYUIOP]^"
    .byte 13,0
    .ascii "ASDFGHJKL\\["
    .byte 0,0
    .ascii "*ZXCVBNM;:-"
    .byte 0,'*,0,32
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '>
    .fill 10,1,0

alt_map:
    .byte 0,0
    .ascii "\0@\0$\0\0{[]}\\\0"
    .byte 0,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte '~,13,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0,0,0
    .fill 16,1,0
    .byte 0,0,0,0,0
    .byte 0,0,0,0,0,0,0
    .byte '|
    .fill 10,1,0

#elif defined(KBD_GR)

key_map:
    .byte 0,27
    .ascii "1234567890\\'"
    .byte 127,9
    .ascii "qwertzuiop@+"
    .byte 13,0
    .ascii "yxcvbnm,.-"
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '<
    .fill 10,1,0

shift_map:
    .byte 0,27
    .ascii "!\"#$%&/()=?`"
    .byte 127,9
    .ascii "QWERtZUIOP\\*"
    .byte 13,0
    .ascii "ASDFGHJKL{}~"
    .byte 0,''
    .ascii "YXCVBNM;:_"
    .byte 0,'*,0,32
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '>
    .fill 10,1,0

alt_map:
    .byte 0,0
    .ascii "\0@\0$\0\0{[]}\\\0"
    .byte 0,0
    .byte '@,0,0,0,0,0,0,0,0,0,0
    .byte '~,13,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0,0,0
    .fill 16,1,0
    .byte 0,0,0,0,0
    .byte 0,0,0,0,0,0,0
    .byte '|
    .fill 10,1,0

#elif defined(KBD_FR)

key_map:
    .byte 0,27
    .ascii "&{\"'(-}_/@)=)"
    .byte 127,9
    .ascii "azertyuiop^$"
    .byte '`,0,42
    .ascii "wxcvbn,;:!"
    .byte 0,'*,0,32
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '<
    .fill 10,1,0

shift_map:
    .byte 0,27
    .ascii "1234567890]+"
    .byte 127,9
    .ascii "AZERTYUIOP<>"
    .byte 13,0
    .ascii "QSDFGHJKLM%"
    .byte '~,0,'#
    .ascii "WXCVBN?./\\"
    .byte 0,'*,0,32
    .fill 16,1,0
    .byte '-,0,0,0,'+
    .byte 0,0,0,0,0,0,0
    .byte '>
    .fill 10,1,0

alt_map:
    .byte 0,0
    .ascii "\0~#{[|`\\^@]}"
    .byte 0,0
    .byte '@,0,0,0,0,0,0,0,0,0,0
    .byte '~,13,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0
    .byte 0,0,0,0,0,0,0,0,0,0,0
    .byte 0,0,0,0
    .fill 16,1,0
    .byte 0,0,0,0,0
    .byte 0,0,0,0,0,0,0
    .byte '|
    .fill 10,1,0

#else
#error "KBD-type not defined"
#endif

/*
**用于处理普通键，即含义没有变化并且只有一个字符返回的键
*/
do_self:
    lea alt_map,%ebx /*alt键同时按下时的映射表基址*/
    testb $0x20,mode
    jne 1f
    lea shift_map,%ebx
    testb $0x03,mode
    jne 1f
    lea key_map,%ebx /*使用普通映射表key_map*/
1:
    movb (%ebx,%eax),%al
    orb %al,%al
    je none
    testb $0x4c,mode
    je 2f
    cmpb $'a,%al
    jb 2f
    cmpb $'},%al
    ja 2f
    subb $32,%al
2:  testb $0x0c,mode
    je 3f
    cmpb $64+32,%al
3:  testb $0x10,mode
    je 4f
    orb $0x80,%al
4:  andl $0xff,%eax
    xorl %ebx,%eax
    call put_queue
none:   ret

minus:  cmpb $1,e0
    jne do_self
    movl $'/,%eax
    jmp put_queue

//下面是一张子程序地址跳转表，当取得扫描码后就根据此表调用
//相应的扫描码处理子程序。大多数调用的子程序是do_self,或者是none
//这起决于是按键make还是释放键break

key_table:
    .long none,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,ctrl,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,lshift,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,do_self,do_self,do_self
    .long do_self,minus,do_self,do_self
    .long alt,do_self,caps,func
    .long func,func,func,func
    .long func,func,func,func
    .long func,num,scroll,cursor
    .long cursor,cursor,do_self,cursor
    .long cursor,cursor,do_self,cursor
    .long cursor,cursor,cursor,cursor
    .long func,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,unctrl,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,unlshift,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,unrshift,none
    .long unalt,none,uncaps,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none
    .long none,none,none,none

kb_wait:
    pushl %eax
1:  inb $0x64,%al
    testb $0x02,%al
    jne 1b
    popl %eax
    ret

reboot:
    call kb_wait
    movw $0x1234,0x482
    movb $0xfc,%al
    outb %al,$0x64
die:    jmp die
