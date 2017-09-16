[bits 32]
startup_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov es, ax

    call setup_idt                  ;调用设置中断描述符表子程序
    call setup_gdt                  ;调用设置全局描述符表子程序
    mov eax, 0x10
    mov ds, ax                      ;因为修改了gdt，所以需要重新装载所有的段
    mov es, ax                      ;寄存器，CS代码段已经在setup_gdt中重新加载过
    mov fs, ax                      ;了
    mov gs, ax

    mov es, ax
    ;;检测A20地址线是否已经开启，采用的方法是向内存地址0x000000处写入任意一个数字
    ;;然后看内存地址0x100000处是否也是这个数字，如果一直相同的话，就一直比较下去
    ;;也即死循环，死机，表示A20线没有选通，结果内核就不能使用1M以上内存
    xor eax, eax
A20:  inc eax
      mov [0x000000], eax
      cmp eax, [0x100000]
      je A20

    ;;注意下面这段程序中，486应将位16置位，以检查超级用户模式下的写保护
    ;;此后verify_area()调用就不需要了。486的用户通常也会将NE（#5）置位
    ;;以便对数学处理器的出错使用int16
    ;;上面原注释中提到的486CPU中cr0控制寄存器的位16是写保护标志WP（write-protect）
    ;;用于禁止超级用户级的程序向一般用户只读页中进行写操作。该标志主要用于操作
    ;;系统创建新进程时实现写时复制（copy-on-write）方法
    ;;下面这段程序永久检查协处理器芯片是否存在，方法数修改控制寄存器cr0，在假设
    ;;存在协处理器的情况下执行一个协处理器指令，如果出错的话说明处理器芯片不存在
    ;;需要设置cr0中的协处理器仿真位EM（），并复位协处理器存在标志MP（位1）
    mov eax, cr0
    and eax, 0x8000001             ;保存PG，PE，ET
    or eax, 2                       ;设置MP位
    mov cr0, eax
    call check_x87
    jmp after_page_tables

    ;;我们依赖于ET标志的正确性来检测287/387是否存在
    ;;下面fninit和fstsw是数学协处理器（80287/80387）的指令
    ;;finit向协处理器发出初始化指令。他会把协处理器置于一个未受以前操作影响的
    ;;已知状态，设置其控制字位默认值，清除状态字和所有浮点栈式寄存器，非等待式
    ;;的这条指令（fninit）还会让协处理器终止执行当前正在执行的任何先前的算术操作，
    ;;fstsw指令取协处理器状态字，如果系统中存在协处理器的话，那么在执行了fninit
    ;;指令后，其状态低字节肯定位0
check_x87:
    fninit                          ;向协处理器发出初始化命令
    fstsw ax                        ;取协处理器状态字到ax寄存器中
    cmp al ,0                      ;初始化后状态字应该位0，否则说明协处理器不
                                    ;存在
    je exist
    mov eax, cr0
    or eax, 6
    mov cr0, eax
    ret
    ;;下面是一汇编语言指示符，其含义是指存储边界对齐调整，“2”表示把随后的代码
    ;;或数据的偏移位置调整到地址值最后2比特位置，即按4字节方式对齐内存地址，不
    ;;过现在GNU as直接写出对齐的值，而非2的次方值了，使用该指示符的目的是为了提
    ;;高32位CPU访问内存中代码或数据的速度和效率
    ;;下面的两个字节值是80287协处理指令fsetpm机器码，其作用是把80287设置位保护
    ;;模式。80387无需该指令，并且会把该指令看做是空操作
exist:
    db 0xdb, 0xe4                  ;287协处理器码
    ret
setup_idt:
    lea edx, [ignore_int]             ;
    mov eax, 0x00080000             ;将选0x0008置入eax的高16位中
    mov ax, dx

    mov dx, 0x8e00

    lea edi, [_idt]
    mov ecx, 256

rp_sidt:
    mov [edi], eax                    ;将哑中断门描述存入表中符 
    mov [edi+4], edx                  ;eax内容放到edi+4所指内存位置处
    add edi,   8
    dec ecx
    jne rp_sidt                     ;加载中断描述符表中下一项
    lidt [idt_descr]                ;加载中断描述符表中下一项
    ret

    ;;设置全局描述符
    ;;这个子程序设置一个新的全局描述符表gdt，并加载。此时仅创建两个表项，与前面的
    ;;该子程序将被覆盖掉

setup_gdt:
    lgdt [gdt_descr]
    ret

    times  4096-($-$$) db 0
    pg0 :
        times 4096 db 1
    pg1:
        times 4096 db 2
    pg2:
        times 4096 db 3
    pg3:
        times 4096 db 4

     ;;定义下面的内存数据从偏移0x5000处开始
_tmp_floppy_area:
    times 1024 db 0                 ;共保留1024项，每项一字节，填充数值0
    ;;下面这几个入栈操作用于跳转到init/main.c中的main()函数做准备工作，第139行
    ;;上的指令在栈中压入返回地址，而第140行则压入main()函数代码的地址，当head.s
    ;;最后在第218行执行ret指令时就会弹出main()的地址，并把控制权转移到init/main.c
    ;;程序中，参见第3章中有关C焊接调用机制说明
    ;;前面3个入栈0值应该分别表示envp，argv指针和argc的值，但main()没用到
    ;;139行的入栈操作是模拟调用main.c程序时首先将返回地址入栈操作，所以如果
    ;;main.c程序真的退出时，就会返回到这里的标号L6处继续执行下去，也即死循环
    ;;140行将main.c得到地址压入堆栈，这样，在设置分页处理(setup_paging)结束后，
    ;;执行ret返回指令时就会将main.c程序的地址弹出堆栈，并执行main.c程序了
after_page_tables:
    push 0
    push 0
    push 0
    push L6
    push _main                  ;_main是编译程序对main 
    jmp setup_paging            ;跳转至第198行
L6:
   jmp L6                       ;mian程序不应该返回到这里 
int_msg:
        db "Unknow interrupt\n\r" ;定义字符串

SECTION mbr align=4
ignore_int:
    push eax
    push ecx
    push edx
    push ds                     ;这里请注意！！ds,es,fs,gs等虽然是16位寄存器，但
                                ;入栈后仍会以32位形式入栈，也需要占用4字节的堆栈
                                ;空间
    push es
    push fs
    mov eax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    push int_msg                ;把调用printk函数的参数(地址)入栈,注意！
                                ;
    call _printk                ;该函数在/kernel/printk.c中

    pop eax
    pop fs
    pop es
    pop ds
    pop edx
    pop ecx
    pop eax
    iret           ;中断返回(把中断调用时压入栈的CPU寄存器（）值也弹出)

    ;;这个子程序通过控制寄存器cr0的标志(PG位31)来启动对内存的分页处理功能
    ;;并设置各个表项的内容，以恒等映射前16MB的物理内存，分页器假定不会产生非法的
    ;;地址映射（也即在只有4MB的机器上设置大于4MB的内存地址）
    ;;
    ;;注意：尽管所以的物理地址都应该由这个子程序进行恒等映射，但只有内核页面管理
    ;;函数能直接使用>1MB地址，所有“普通”函数仅使用低于1MB的地址空间。或者使用局部
    ;;数据空间，该地址空间将被映射到其它一些地方去-mm(内存管理程序)会管理这些事的

    ;;对于有些多余16MB的家伙，通常只需要修改一些常数等
    ;;机器物理内存中大于1MB的内存空间主要被用于主内存区
    ;;主内存区空间由mm模块管理，它涉及到页面映射操作，内核中所有其它函数就是这里
    ;;指的一般(普通)函数.若要使用主内存的页面，就需要使用get_free_page()等函数来
    ;;获取，因为主内存区中内存页面是共享资源，必须有程序进行统一管理，以避免资源
    ;;征用和竞争

    ;;在内存物理地址0x0处开始存放1页页目录和4页页表,页目录表是系统所有进程公用的
    ;;而这里的4页页表则属于内核专用，他们一一映射线性地址起始16MB空间范围到物理
    ;;内存上，对于新的进程，系统会在内存分区为其申请页面存放页表，另外，1页内存
    ;;长度是4096字节

    ;按4字节方式对齐内存地址边界
SECTION mbr align=4
setup_paging :           ;先对5页内存（1页目录+4页页表）清0
    mov ecx, 1024*5
    xorl eax, eax       ;异或指令相同为0不同为1
    xorl edi, edi

    cld                 ;清除方向位DF ,ESI和EDI递增。std设置DF标志，ESI和EDI递减
    rep                 ;先查看ecx是否为0，如果不为0，ecx减1，重复其后边操作
   ;eax内容到es：edi所指内存位置处，且edi增4
    stosl

    ;;下面4句设置目录表中的项，因为我们（内核）共有4个页表，所以只需设置4项
    ;;页目录项的结构与页表中项的结构一样，4字节为一项
    ;;例如"pg0+1"表示：0x00001007，是页目录表中的第一项
    ;;则第一个页表属性标志=0x00001007&0xfffff000=0x1000
    ;;第一个页表的属性标志=0x00001007&0x00000fff=0x07表示该页存在，用户可读写
    mov ax, pg0+7
    mov [_pg_dir], ax
    mov ax, pg1+7
    mov [_pg_dir+4], ax
    mov ax, pg2+7
    mov [_pg_dir+8], ax
    mov ax, pg3+7
    mov [_pg_dir+12], ax

    ;;下面6行填写4个页表中所有项的内容，共有4（页表）×1024（项/页表）=4096（0-0xff）
    ;;也即能映射物理内存4096*4KB=16MB
    ;;每页的内容是：当前项所映射的物理内存地址+该页的标志（这里均为7）
    ;;使用方法是从最后一个页表的最后一项开始按倒退顺序填写，一个页表的最后一项在
    ;;页表的最后一项在页表中的1023*4=4092,因此最后一项的位置是pg3+4092
    mov edi, [pg3+4092]           ;edi  最后一页的最后一项的
    mov eax, 0xfff007           ;最后一项对应物理内存页面的地址是0xfff000
                                ;加上属性标志7，即为0xfff07
    std                         ;方向置位，edi值减（四字节）
label:
    stosl
     cld                         ;edi+4
     sub eax, 0x1000             ;每填写好一项，物理地址减0x1000
     jge label                   ;如果小于0则说明全填写好了
    ;;设置页目录表基址寄存器cr3的值，指向页目录表，cr3中保存的是页目录表的物理地址
    xor eax, eax                ;页目录表在0x0000处
    mov cr3, eax
    ;;设置启动使用分页处理（cr0的PG标志，位31）
    mov eax, cr0
    xor eax, 0x80000000
    mov cr0, eax
    ret

    ;按4字节方式对齐内存地址边界
SECTION mbr align=4
dw 0                         ;这里先空出2字节，这样224行上的长字是4字节对齐的
    ;;下面是加载中断描述符表寄存器idtr的指令lidt要求的6字节操作数，前2字节是idt
    ;;表的限长，后4字节是idt表在线性地址空间的32位基地址
gdt_descr:
    dw 256*8-1              ;gdt的限长
    dq _gdt
idt_descr:
    dw 256*8-1
    dq _gdt

    ;按8（2的3次方）字节方式对齐内存四周边界
SECTION mbr align=8
_idt: times 256*8 db 0           ;256项每项8字节，填0
;;全局表，前4项分别是空项（不用）,代码段描述符，数据段描述符,系统段描述符，其中
;;系统段调用段描述符并没有派用处，linus当时可能想把系统调用代码专门放在这个独立的
;;段中，后面还预留252项空间，用于放置在所厂家任务的局部描述符（）和对应的任务状态
;;段TSS的描述符
;;()
_gdt:
      dq 0x0000000000000000      ;空描述符
      dq 0x00c09a0000000fff      ;16MB  0x08，内核代码段最大长度16MB
      dq 0x00c0920000000fff      ;16MB  0x10 ,内核数据段最大长度16MB
      dq 0x0000000000000000
      times 252*8 db 0               ;预留空间
_main: db 0
_printk : db 0
_pg_dir : db 0
