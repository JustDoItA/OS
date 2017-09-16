    org 07c00h          ; 告诉编译器程序加载到7c00处
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov dx, ax

    ;将该程序搬迁至段基址为INITSEG处
    mov ax, BOOTSEG
    mov ds, ax
    mov ax, INITSEG
    mov es, ax
    mov cx, 256
    sub si, si
    sub di, di
    rep
    movsw

    ;call   DispStr         ; 调用显示字符串例程
    jmp INITSEG:go-0x7c00
    jmp $           ; 无限循环
go:
    mov ax, cs
    mov es, dx
    mov ax, msg1
    mov bp, ax          ; ES:BP = 串地址
    mov cx, 17          ; CX = 串长度
    mov ax, 01301h      ; AH = 13,  AL = 01h
    mov bx, 000ch       ; 页号为0(BH = 0) 黑底红字(BL = 0Ch,高亮)
    mov dl, 0
    int 10h         ; 10h 号中断

    ;;利用BIOS中断INT 0x13将setup模块从磁盘第二个扇区读到0x90200开始处，共读
    ;;4个扇区，如果读出错，则复位驱动器，并读扇区
load_setup:
    mov ax, 0x9000
    mov es, ax
    mov ds, ax
    mov dx, 0x0000      ;驱动0 头部0
    mov cx, 0x0002      ;2扇区 0磁道
    mov bx, 0x0200      ;基址512
    mov ax, 0x0200+SETUPLEN     ;
    int 0x13            ;向运行内存中读取数据
    jnc ok_load_setup   ;cf进位标志符，即满足进位标志符就跳转
    mov dx, 0x0000      ;DL需要复位的驱动器号
    mov ax, 0x0000      ;重置磁盘，当磁盘IO功能调用出现错误时需要用到此功能,
                        ;BIOS检查磁盘状态，并将磁头校准磁头使之在应该在的位置上
    int 0x13
    jmp load_setup

    ;;取磁盘驱动器的参数，特别是每磁道的扇区数量
ok_load_setup:
    mov dl, 0x00        ;驱动器数量
    mov ax, 0x0800      ;ah=8 设备驱动号0x08
    int 0x13
    mov ch, 0x00        ;cx高8位清0
    mov [sectors-0x7c00], cx     ;保存每磁道扇区数cx低8位的值,软盘18扇区
    ;mov ax, INITSEG
    ;mov es, ax          ;因为上面取磁盘参数中断改掉了es的值，这里重新改回

    ;;将system模块加载到0x1000(64k）开始处
    mov ax, SYSSEG
    mov es, ax
    call read_it         ;读磁盘上system模块es为输入参数
    call kill_motor     ;关闭马达，这样就可以知道驱动昂器的状态了

    ;;此后我们检查要使用那个根文件系统（简称根设备），如果已经指定了设备（!=0）
    ;;就直接使用给定的设备。否则就需要根据BIOS报告每个磁道扇区数来确定到底使用
    ;;/dev/PS0（2,28）还是/dev/at0（2,8）
    mov ax, [ROOT_DEV-0x7c00]    ;取508,509字节处的根设备号并判断是否已被定义
    cmp ax, 0x0
    jne root_defined
    ;; 取上面保存的每磁道扇区数，如果secotrs处的值位15，则说明是1.2MB驱动器；
    ;;如果secotrs处的值为18则说明是1.44MB软驱。因为是可引导的驱动器，所以肯定
    ;;是A驱
    mov bx, [sectors-0x7c00]
    mov ax, 0x0208              ;/dev/ps0 - 1.2MB
    cmp bx, 0x0f
    je root_defined             ;如果等于，则ax中就是引导驱动器的设备号
    mov ax, 0x021c              ;/dev/PS0 - 1.44MB
    cmp bx, 0x12
    je root_defined
undef_root:                     ;如果不一样，则死循环（死机）
    jmp undef_root
root_defined:
    mov [ROOT_DEV-0x7c00], ax   ;将检查过的设备号保存到ROOT_DEV中
    ;;到此所有程序都加载完毕。跳转到0x9020:0000(setup.s)去执行
jmp SETUPSEG:0000

    ;;首先测试输入的段值，从磁盘读入的数据必须存在位于内存64kb的边界处
read_it:
    mov ax, SYSSEG
    mov es, ax
    test ax, 0xfff          ;以比特位逻辑于两个操作数，若两个数对于的比特位
                            ;都为1，则结果值对应的比特位为1，否则为0，该操作结果
                            ;只影响标志（ZF）
                            ;ZF = 1 0x1000&0x0fff
die:    jne die     ;ZF为0转至die标签
        xor bx, bx  ;bx位段内偏移
    ;;接着判断是否已经读入全部数据，比较当前所读是够就是系统数据末端（ENDSEG）,
    ;;如果不是则跳转至下面ok1_read标号处继读数据。否则退出子程序返回
rp_read:
    mov ax, es
    cmp ax, ENDSEG          ;是否已经加载了全部数据
    jb ok1_read             ;ax小于ENDSEG  根据标志位CF=1如果cf小于1则说明有借位
    ret                     ;返回read_it标签
ok1_read:
    ;;计数和验证当前磁道要读取的扇区数，放在ax寄存器中
    ;;根据当前磁道还未读取的扇区数以及段内数据字节开始的偏移位置，计算如果全部
    ;;读取这些未读扇区，所读总字节数是否会超过64KB段长度的限制，若会超过，则根据
    ;;此次最多能读入字节数（64KB段内偏移地址),反算出此次需要读取的扇区数
    mov ax, [sectors-0x7c00]         ;取每磁道扇区数
    sub ax, [sread-0x7c00]           ;减去当前磁道已读扇区数
    mov cx, ax                       ;cx现在为当前未读扇区数
    shl cx, 9                        ;cx=cx*512
    add cx, bx                       ;cx+段内共读入的字节数
    jnc ok2_read                     ;若没有超过64kb字节，则跳转至ok2_read处执行
    je ok2_read			     ;jnc无进位时转移，je运算结果位0即CF=1时转移	
    ;;若加上次将读磁道上所有未读扇区时会超过64k，则计算此时最多能读入的字节数
    ;;64k-段内读偏移位置，再转换成需要读取的扇区数,其中0减去某数就是读取64kb的
    ;;补值
    xor ax, ax
    sub ax, bx
    shr ax, 9

ok2_read:
    ;;读取当前磁道上指定开始扇区cl和需要读取扇区al的数据到es：bx处，然后统计当前
    ;;磁道上已经读取的扇区数并与磁道最大扇区数sectors做比较，如果小于sectors说明
    ;;当前磁道扇区上还有未读扇区，于是跳转到ok3_read继续执行处
    call read_track         ;读取当前磁道上指定开始扇区和需读取扇区的数据
    mov cx, ax              ;cx = 该次操作已读取的扇区数
    add ax, [sread-0x7c00]  ;加上当前磁道上已经读取的扇区数
    ;
    cmp ax, [sectors-0x7c00] ;如果当前磁道上还有扇区未读，则跳转到ok3_read处
    jne ok3_read
    ;;若该磁道的当前磁头面所有扇区已读，则读取该磁道的下一磁头面，
    ;;（1号磁头）上的数据,如果已经完成，则去读下一磁道
    mov ax, 1                   ;判断当前磁头号
    sub ax, [head-0x7c00]       ;判断当前磁头号
    jne ok4_read                ;如果是0磁头，则再去读1磁头面上的扇区数据， zf=0
                                ;转至ok4_read处执行
                                ;如果已经完成，则区读下一磁道
    inc word [track-0x7c00]     ;[track]否则去读下一磁道
ok4_read:
    mov [head-0x7c00], ax        ;保存当前磁头号
    xor ax, ax                   ;清当前磁道已读数据


    ;;如果当前磁道上还有未读扇区，则首先保存当前磁道已读的扇区数，然后调整存放
    ;;数据处的开始位置。若小于64kb边界值，则跳转到rp_read处，继续读数据
ok3_read:
    mov [sread-0x7c00], ax       ;保存当前磁道已读扇区数
    shl cx, 9                    ;上去已读扇区数×512字节
    add bx, cx                   ;调整当前段内数据的开始位置
    jnc rp_read
    ;;否则说明已经读取64KB数据。此时调整当前段，为读下一段做准备
    mov ax, es
    add ax, 0x1000
    mov es, ax
    xor bx, bx          ;清段内数据开始偏移值
    jmp rp_read

read_track:
    push ax                 ;13扇区
    push bx                 ;0
    push cx                 ;13*512=6656b
    push dx
    mov dx, [track-0x7c00]  ;取当前磁道号 0
    mov cx, [sread-0x7c00]  ;取当前磁道上已读扇区数 5
    inc cx                  ;cl = 开始读扇区        6
    mov ch, dl              ;ch = 当前磁道号
    mov dx, [head-0x7c00]   ;取当前磁头号
    mov dh, dl              ;dh = 磁头号            dx=0x0000
    mov dl, 0               ;dl = 驱动器号 0表示当前A驱动器
    and dx, 0x0100          ;磁头号不大于1
    mov ah, 0x02            ;ah = 2,读磁盘扇区功能号
    int 0x13
    jc bad_rt               ;若出错则跳转至bad_rt
    pop dx
    pop cx
    pop bx
    pop ax
    ret
    ;;读磁盘操作出错，则执行驱动器复位操作（磁盘中断功能号0)，再跳转到read_track处重试
bad_rt:
    mov ax, 0
    mov dx, 0
    int 0x13
    pop dx
    pop cx
    pop bx
    pop ax
    jmp read_track

kill_motor:
    push dx
    mov dx, 0x3f2                   ;软驱控制卡的数字输出寄存器（DOR）端口，只写
    mov al, 0
    out dx, al                      ;将al中的内容输出到dx指定的端口去
    pop dx
    ret                             ;kill_motor标签

msg1:       db  "Loading system..."
sectors: dw  0x0
track :  dw  0x0     ;当前磁道号
head  :  dw  0x0     ;当前磁头号
sread : dw  1+SETUPLEN   ;当前磁道中已读扇区数
BOOTSEG  equ 0x07c0
INITSEG  equ 0x9000
SETUPSEG equ 0x9020
SYSSEG equ 0x1000
SYSSIZE equ 0x8000
ENDSEG equ SYSSEG + SYSSIZE
SETUPLEN equ 4

times   508-($-$$)  db  0   ; 填充剩下的空间，使生成的二进制代码恰好为512字节

ROOT_DEV : dw 0x306
dw  0xaa55                  ; 结束标志
