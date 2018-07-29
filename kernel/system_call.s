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

.global system_call,sys_fork,timer_interrupt,sys_execve
.global hd_interrupt,floppy_interrupt,parallel_interrupt
.global device_not_available, coprocessor_error

.align 4
bad_sys_call:
    movl $-1, %eax
    iret

.align 4
reschedule:
    pushl $ret_from_sys_call    #将ret_from_sys_call的地址入栈 
    jmp schedule

.align 4
system_call:
    cmpl $nr_system_calls-1, %eax   #调用号如果超出范围的话 就在eax中置-1并退出
    ja bad_sys_call
    push %ds            #保存原段寄存器值
    push %es
    push %fs
    pushl %edx          #ebx,ecx,edx 中放着系统调用相应的C语言函数的调用参数
    pushl %ecx
    pushl %ebx
    movl  $0x10, %edx
    mov %dx,%ds         #ds,es 指向内核数据段(全局描述符表中数据段描述符)
    mov %dx,%es
    movl $0x17, %edx    #fs 指向局部数据段(局部描述符表中数据段描述符)
    mov %dx, %fs
#下面这句操作数的含义是: 调用地址 = sys_call_table +  %eax * 4
#对应C程序中的sys_call_table在include/linux/sys.h中,其中定义了一个包括72个系统
#调用C处理函数的地址数组表
    call *sys_call_table(,%eax,4)
    #call sys_call_table(,%eax,4)
#把系统调用号入栈
    pushl %eax
    movl current,%eax #取当前任务(进程) 数据结构地址 eax
#下面功能是查看当前任务的运行状态。如果不在就绪状态(state != 0)
#就去执行调度程序
    cmpl $0,state(%eax)
    jne reschedule
    cmpl $0,counter(%eax)
    je reschedule

#以下这段代码从系统调用C函数返回后,对信号量进行识别处理
ret_from_sys_call:
    movl current,%eax       #首先判别当前任务是否是初始任务task0,如果
    #是则不必对其进行信号量方面的处理,直接返回
    cmpl task,%eax
    je 3f  #向前跳转到标志号3处
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

parallel_interrupt:
    pushl %eax
    movb $0x20,%al
    outb %al,$0x20
    popl %eax
    iret

.align 4
sys_execve:
    pushl %eax
    ret

.align 4
sys_fork:
    call find_empty_process
    #不改变%eax值却可以设置或者清除一些标志位，一遍实现分支结构，而test则是根据
    #上次计算结果的性质来设置相应的状态码，testl的结果就是%eax，于是就针对%eax的值
    #属性设置状态码
    testl %eax, %eax #将两个操作数做与来设置0标志位和负数标识(此处是为了检查eax值)
    js 1f
    push %gs
    pushl %esi
    pushl %edi
    pushl %ebp
    pushl %eax
    call copy_process
    addl $20, %esp #丢弃所有的压栈内容
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
