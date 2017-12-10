    mov ax, INITSEG
    mov ds, ax
    mov ah, 0x03
    xor bh, bh
    int 0x10
    mov [0], dx                 ;dh-行号 dl-列号（0行17列）
    mov ah, 0x88
    int 0x15
    mov [2], ax                 ;将扩展内存存在0x90002处（32MB）

    ;;显示当前显示模式
    mov ah, 0x0f
    int 0x10
    mov [4], bx                 ;存放当前页（当前显示0页）
    mov [6], ax                 ;存放显示模式，当前列数（50|03->80列|16色字符模式 80*25）

    ;;检查显示方式(EGA/VGA)，并获取参数
    mov ah, 0x12
    mov bl, 0x10
    int 0x10
    mov [8], ax			;ax=0x1203
    mov [10], bx                ;安装的显示内存及状态（彩色/单色）已安装显示内存大小03=256k
    mov [12], cx                ;显示卡特性参数（EGA-80*25）

    ;;分别复制有关两个硬盘的参数表，0x90080处存放第一个硬盘的表
    ;;0x90090用于存放第二个硬盘的表
    mov ax, 0x0000
    mov ds, ax
    lds si, [4*0x41]            ;取中断向量0x41的值，也即hd0参数表的地址->ds:si
				;把[ds:4*0x41]处存放的32位偏移地址取出存放在ds：si
    mov ax, INITSEG
    mov es, ax
    mov di, 0x80                ;传送的目的地址： 0x9000:0x0080->es:di
    mov cx, 0x10                ;共传输16字节
    rep
    movsb

    mov ax, 0x0000
    mov ds, ax
    lds si, [4*0x46]            ;取中断向量0x46的值，也即hd1参数表的地址->ds:si
    mov ax, INITSEG
    mov es, ax
    mov di, 0x0090
    mov cx, 0x10
    rep
    movsb

    ;;检查系统是否有第二个硬盘，如果没有则把第二个表清0
    mov ax, 0x01500
    mov dl, 0x81
    int 0x13
    jc no_disk1
    cmp ah, 3
    je is_disk1
    ;;如果第2个硬盘不存在，则对第2个硬盘表清0
no_disk1:
    mov ax, INITSEG
    mov es, ax
    mov di, 0x0090
    mov cx, 0x10
    mov ax, 0x00
    rep
    stosb
;;进入保护模式...
is_disk1:
    cli                         ;从此开始不运行中断
    mov ax, 0x0000
    cld                         ;DF方向标志位复位 即DF=0
do_move:
    mov es, ax                  ;es:di 时目的地址初始为 0x0:0x0
    add ax, 0x1000
    cmp ax, 0x9000              ;是否已经把最后一段（从0x8000段开始的64K）移动完
    jz  end_move                ;是，则跳转
    mov ds, ax                  ;ds:si是源地址
    sub di, di
    sub si, si
    mov cx, 0x8000              ;移动0x8000字(64K字节)
    rep
    movsw
    jmp do_move
end_move:
    mov ax, SETUPSEG
    mov ds, ax
    lidt [idt_48]
    lgdt [gdt_48]

    ;;打开A20地址线
    call empty_8042        ;测试8042状态寄存器，等待输入缓冲器空
                            ;只有当输入缓冲器为空时才可以对起进行写命令
    mov al, 0xd1            ;0xd1命名码表示要写数据到8024的P2端口上。
                            ;P2端口位1用于A20线的选通
                            ;数据要写到0x60口
    out 0x64, al

    call empty_8042        ;等待输入缓冲器空，看命令是否被接受
    mov al, 0xdf           ;选通A20地址线参数
    out 0x60, al
    call empty_8042        ;若此时输入缓冲器为空，则表示A20线已选通

    ;;8259芯片主片端口是0x20-0x21,从片端口是0xa0-x0xa1,输出值0x11表示初始化命令开始
    ;; 它是ICW1命令字，表示边沿触发，多片8259级连，最后要发送ICW4命令字
    mov al, 0x11
    out 0x20, al            ;发送到8259A-1主芯片
    dw 0x00eb, 0x00eb       ;表示
    out 0xa0, al
    dw 0x00eb, 0x00eb
    mov al, 0x20
    out 0x21, al
    dw 0x00eb, 0x00eb
    mov al, 0x28
    out 0xa1, al
    dw 0x00eb, 0x00eb
    mov al, 0x04
    out 0x21, al
    dw 0x00eb, 0x00eb
    mov al, 0x02
    out 0xa1, al

    dw 0x00eb, 0x00eb
    mov al, 0x01
    out 0x21, al

    dw 0x00eb, 0x00eb
    out 0xa1, al

    dw 0x00eb, 0x00eb
    mov al, 0xff
    out 0x21, al
    dw 0x00eb, 0x00eb
    out 0xa1, al

    ;;这里设置进入32位保护模式，首先加载机器状态字，也称控制寄存器CR0，起bit位0
    ;;置1也将导致cpu工作在保护模式
    mov ax, 0x0001          ;保护模式比特位（PE）
    lmsw ax
;    mov cx, 0x08            ;跳转至cs段8，偏移0处
    jmp dword 0x08:0x00

    empty_8042:
        dw 0x00eb, 0x00eb   ;直接使用机器码表示两条相对跳转指令，起延时作用
        in    al, 0x64
        test  al, 2
        jnz   empty_8042
        ret
    gdt:
             dw 0,0,0,0 ;NULL空描述符
             ;系统代码段,偏移量为0x80
             dw 0x07ff  ;段界限0x07ff=2047
                        ;0~2047共计2048*(4K*1024)=8M
             dw 0x0000  ;段基址
             dw 0x9a00  ;段属性 读/执行
             dw 0x00c0  ;
            ;系统的数据段
             dw 0x07ff
             dw 0x0000
             dw 0x9200
             dw 0x00c0
    idt_48   dw 0
             dw 0,0
    gdt_48   dw 0x800           ;描述符表的长度 256(实体)*8(字节)=2048=0x800
             dw 512+gdt,0x9     ;gdt的线性地址512(引导扇区的长度)
             ; +gdt(gdt的线性地址),0x9(加载boot时将该部分程序移动至0x90000后)
    INITSEG  equ 0x9000
    SYSSEG   equ 0x1000
    SETUPSEG equ 0x9020

times 2046-($-$$) db 'a'
dw 0xaa55
