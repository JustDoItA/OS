/*
  @lpf
  Sun Nov  5 01:49:43 PST 2017
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/system.h>


#define ORIG_X (*(unsigned char *)0x90000)
#define ORIG_Y (*(unsigned char *)0x90001)

#define ORIG_VIDEO_PAGE (*(unsigned short *)0x90004)
#define ORIG_VIDEO_MODE ((*(unsigned short *)0x90006) & 0xff)
#define ORIG_VIDEO_COLS (((*(unsigned short *)0x90006) & 0xff00) >> 8)
#define ORIG_VIDEO_LINES (25)

#define ORIG_VIDEO_EGA_BX (*(unsigned short *)0x9000a)


#define VIDEO_TYPE_MDA 0x10
#define VIDEO_TYPE_CGA 0x11
#define VIDEO_TYPE_EGAM 0x20
#define VIDEO_TYPE_EGAC 0x21

#define NPAR 16

extern void keyboard_interrupt(void);

static unsigned char video_type;
static unsigned long video_num_columns;
static unsigned long video_size_row;
static unsigned long video_num_lines;
static unsigned char video_page;
static unsigned long video_mem_start;
static unsigned long video_mem_end;


static unsigned short video_port_reg;
static unsigned short video_port_val;
static unsigned short video_erase_char;

static unsigned long origin;
static unsigned long scr_end;
static unsigned long pos;
static unsigned long x,y;
static unsigned long top,bottom;
static unsigned long state = 0;
static unsigned long npar,par[NPAR];
static unsigned long ques = 0;
static unsigned char attr = 0x07;

//int beepcount = 0;

void sysbeep(void){
    outb_p(inb_p(0x61)|3, 0x61);
    outb_p(0xB6, 0x43);
    outb_p(0x37, 0x42);
    outb(0x06, 0x42);
    //beepcount = HZ/8;
}

static inline void gotoxy(unsigned int new_x, unsigned new_y){
    if(new_x > video_num_columns || new_y >= video_num_lines){
        return;
    }
    x = new_x;
    y = new_y;
    pos = origin + y*video_size_row + (x<<1);
}

static inline void set_origin(){
    cli();
    outb_p(12, video_port_reg);
    outb_p(0xff & ((origin - video_mem_start)>>9), video_port_val);
    outb_p(13, video_port_reg);
    outb_p(0xff & ((origin - video_mem_start)>>1), video_port_val);
    sti();
}

static void scrup(void){
    if(video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM){
        if(!top && bottom == video_num_lines){
            origin += video_size_row;
            pos += video_size_row;
            scr_end += video_size_row;
            if(scr_end > video_mem_end){
                __asm__("cld \n\t"
                    "rep\n\t"
                    "movsl\n\t"
                    "movl video_num_columns,%1\n\t"
                    "rep\n\t"
                    "stosw"
                        ::"a" (video_erase_char),
                         "c" ((video_num_lines - 1) * video_num_columns>>1),
                         "D" (video_mem_start),
                         "S" (origin)
                    );//:"cx","di","si");
                scr_end -= origin - video_mem_start;
                pos -= origin - video_mem_start;
                origin = video_mem_start;
            }else{
                __asm__("cld\n\t"
                    "rep\n\t"
                    "stosw"
                        ::"a" (video_erase_char),
                         "c" (video_num_columns),
                         "D" (scr_end - video_size_row)
                    );//:"cx","di");
            }
            set_origin();
        }else{
            __asm__("cld\n\t"
                "rep\n\t"
                "movsl\n\t"
                "movl video_num_columns, %%ecx\n\t"
                "rep\n\t"
                "stosw"
                ::"a" (video_erase_char),
                "c" ((bottom - top-1) * video_num_columns>>1),
                "D" (origin + video_size_row * top),
                "S" (origin + video_size_row * (top+1))
                );//:"cx","di","si");
        }
    }else{
        __asm__("cld\n\t"
            "rep\n\t"
            "movsl\n\t"
                "movl video_num_columns, %%ecx\n\t"
                "rep\n\t"
                "stosw"
                ::"a" (video_erase_char),
                 "c" ((bottom - top - 1) * video_num_columns>>1),
                 "D" (origin + video_size_row * top),
                 "S" (origin + video_size_row * (top + 1))
            );//:"cx","di","si");
    }
}

static void scrdown(void){
    if(video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM){
        __asm__("std\n\t"
            "rep\n\t"
            "movsl\n\t"
            "addl $2,%%edi\n\t"
            "rep\n\t"
            "stosw"
                ::"a" (video_erase_char),
                 "c" ((bottom - top -1) * video_num_columns>>1),
                 "D" (origin + video_size_row * bottom - 4),
                 "S" (origin + video_size_row * (bottom - 1) - 4)
            );//:"ax","cx","di","si");
    }else{
        __asm__("std\n\t"
            "rep\n\t"
            "addl $2, %%edi\n\t"
            "movl video_num_columns, %%ecx\n\t"
            "rep\n\t"
            "stosw"
                ::"a" (video_erase_char),
                 "c" ((bottom - top - 1) * video_num_columns>>1),
                 "D" (origin + video_size_row * bottom - 4),
                 "S" (origin + video_size_row * bottom - 4)
            );//:"ax","cx","di","si");
    }
}

static void lf(void){
    if(y + 1 < bottom){
        y++;
        pos += video_size_row;
        return;
    }
    scrup();
}

static void ri(void){
    if(y>top){
        y--;
        pos -= video_size_row;
        return;
    }
    scrdown();
}

static void cr(void){
    pos -= x<<1;
    x = 0;
}

static void del(void){
    if(x){
        pos -= 2;
        x--;
        *(unsigned short *)pos = video_erase_char;
    }
}

static void csi_J(int par){
    //long count __asm__("cx");
    //long start __asm__("di");
    long count ;//__asm__("mov count, %cx");
    long start ;//__asm__("mov start, %di");

    switch(par){
    case 0:
        count = (scr_end - pos)>>1;
        start = pos;
        break;
    case 1:
        count = (pos - origin)>>1;
        start = origin;
        break;
    case 2:
        count = video_num_columns * video_num_lines;
        start = origin;
        break;
    default:
        return;
    }
    __asm__("cld\n\t"
        "rep\n\t"
        "stosw\n\t"
            ::"c" (count),
             "D" (start),"a" (video_erase_char)
        );//:"cx","di");
}

static void csi_K(int par){
    long count ;//__asm__("mov count, %cx");
    long start ;//__asm__("mov start, %di");

    switch(par){
    case 0:
        if(x >= video_num_columns){
            return;
        }
        count = video_num_columns - x;
        start = pos;
        break;
    case 1:
        start = pos - (x<<1);
        count = (x < video_num_columns)?x:video_num_columns;
        break;
    case 2:
        start = pos - (x<<1);
        count = video_num_columns;
        break;
    default:
        return;
    }
    __asm__("cld\n\t"
        "rep\n\t"
        "stosw\n\t"
            ::"c" (count),
             "D" (start),"a" (video_erase_char)
        );//:"cx","di");
}
void csi_m(void){
    int i;

    for (i = 0; i <= npar; i++){
        switch(par[i]){
        case 0: attr = 0x07;break;
        case 1: attr = 0x0f;break;
        case 4: attr = 0x0f;break;
        case 7: attr = 0x70;break;
        case 27: attr = 0x07;break;
        }
    }
}

static inline void set_cursor(void){
    cli();
    outb_p(14, video_port_reg);
    outb_p(0xff&((pos - video_mem_start)>>9),video_port_val);
    outb_p(15, video_port_reg);
    outb_p(0xff&((pos - video_mem_start)>>1), video_port_val);
    sti();
}

static void respond(struct tty_struct *tty){}

static void insert_line(void){}

static void delete_char(void){}

static void delte_line(void){}

static void csi_at(unsigned int nr){}

static void csi_L(unsigned int nr){}

static void csi_P(unsigned int nr){}

static void csi_M(unsigned int nr){}

static int saved_x = 0;
static int saved_y = 0;

static void save_cur(){
    saved_x = x;
    saved_y = y;
}

static void restore_cur(void){
    gotoxy(saved_x, saved_y);
}

void con_write(struct tty_struct *tty){
    int nr;
    char c;

    nr = CHARS(tty->write_q);
    while(nr--){
        GETCH(tty->write_q,c);
        switch(state){
        case 0:
            if(c > 31 && c < 127){
                if(x >= video_num_columns){
                    x -= video_num_columns;
                    pos -= video_size_row;
                    lf();
                }
                __asm__("movb attr, %%ah\n\t"
                    "movw %%ax, %1\n\t"
                        ::"a" (c), "m" (*(short *)pos)
                    );//:"ax");
                pos += 2;
                x++;
            }else if (c==27){
                state = 1;
            }else if(c==10 || c==11 || c==12){
                lf();
            }else if(c == 13){
                cr();
            }else if (c==ERASE_CHAR(tty)){
                del();
            }else if(c==8){
                if(x){
                    x--;
                    pos -= 2;
                }
            }else if(c==9){
                c = 8-(x&7);
                x += c;
                pos += c<<1;
                if(x>video_num_columns){
                    x -= video_num_columns;
                    pos -= video_size_row;
                    lf();
                }
                c=9;
        }else if(c==7){
            sysbeep();
        }
        break;
    case 1:
        state = 0;
        if(c == '['){
            state = 2;
        }else if(c == 'E'){
            gotoxy(0, y+1);
        }else if(c=='M'){
            ri();
        }else if(c=='D'){
            lf();
        }else if(c=='Z'){
            respond(tty);
        }else if(x=='7'){
            save_cur();
        }else if(x=='8'){
            restore_cur();
        }
        break;
    case 2:
        for(npar = 0; npar < NPAR; npar++){
            par[npar] = 0;
        }
        npar = 0;
        state = 3;
        if((ques = (c == '?'))){
            break;
        }
    case 3:
        if(c == ';' && npar < NPAR - 1){
            npar++;
            break;
        }else if(c >='0' && c<='9' ){
            par[npar] = 10*par[npar] + c - '0';
            break;
        }else {
            state = 4;
        }
    case 4:
        state = 0;
        switch(c){
        case 'G':
        case '`':
            if(par[0]){
                par[0]--;
            }
            gotoxy(par[0], y);
            break;
        case 'A':
            if(!par[0]){
                par[0]++;
            }
            gotoxy(x, y - par[0]);
            break;
        case 'B':
        case 'e':
            if(!par[0]){
                par[0]++;
            }
            gotoxy(x, y + par[0]);
            break;
        case 'D':
            if(!par[0]++){
                par[0]++;
            }
            gotoxy(x + par[0], y);
            break;
        case 'E':
            if(!par[0]){
                par[0]++;
            }
            gotoxy(0, y + par[0]);
            break;
        case 'F':
            if(!par[0]){
                par[0]++;
            }
            gotoxy(0, y - par[0]);
            break;
        case 'd':
            if(!par[0]){
                par[0]--;
            }
            gotoxy(x, par[0]);
            break;
        case 'H':
        case 'f':
            if(par[0]){
                par[0]--;
            }
            if(par[1]){
                par[1]--;
            }
            gotoxy(par[1], par[0]);
            break;
        case 'J':
            csi_J(par[0]);
            break;
        case 'K':
            csi_K(par[0]);
            break;
        case 'L':
            csi_L(par[0]);
            break;
        case 'M':
            csi_M(par[0]);
            break;
        case 'P':
            csi_P(par[0]);
            break;
        case '@':
            csi_at(par[0]);
            break;
        case 'm':
            csi_m();
            break;
        case 'r':
            if (par[0]){
                par[0]--;
            }
            if(!par[1]){
                par[1] = video_num_lines;
            }
            if(par[0] < par[1] && par[1] <= video_num_lines){
                top = par[0];
                bottom = par[1];
            }
            break;
        case 's':
            save_cur();
            break;
        case 'u':
            restore_cur();
            break;
        }
        }
    }
    set_cursor();
}

void con_init(void){
    register unsigned char a;
    char *display_desc = "????";
    char *display_ptr;

    video_num_columns = ORIG_VIDEO_COLS; //显示器显示字符列数(80)
    video_size_row = video_num_columns * 2; //每行需要使用的字节数(160)
    video_num_lines = ORIG_VIDEO_LINES; //显示器显示字符行数(25)
    video_page = ORIG_VIDEO_PAGE; //当前显示页面 (0)
    video_erase_char = 0x0720; //擦除字符（0x20显示字符，0x07属性）

    //ORIG_VIDEO_MODE = 7 单色显示器
    if(ORIG_VIDEO_MODE == 7){
        video_mem_start = 0xb0000;  //设置单显影像内存起始地址
        video_port_reg = 0x3b4;    //设置单显索引寄存器端口
        video_port_val = 0x3b5;    //设置单显数据寄存器端口
        if((ORIG_VIDEO_EGA_BX & 0xff) != 0x10){
            video_type = VIDEO_TYPE_EGAM; //设置显示类型EGA单色 (33 '!')
            video_mem_end = 0xb8000;      //设置显示内存末端地址
            display_desc = "EGAm";       //设置显示描述字符串
        }else{
            video_type = VIDEO_TYPE_MDA;
            video_mem_end = 0xb2000;
            display_desc = "*MDA";
        }
    }else{
        video_mem_start = 0xb8000;
        video_port_reg = 0x3d4;
        video_port_val = 0x3d5;
        if((ORIG_VIDEO_EGA_BX & 0xff) != 0x10){
            video_type = VIDEO_TYPE_EGAC;
            video_mem_end = 0xbc000;
            display_desc = "EGAc";
        }else{
            video_type = VIDEO_TYPE_CGA;
            video_mem_end = 0xba000;
            display_desc = "*CGA";
        }
    }

    //在屏幕右上角显示描述字符串，采用的方法是直接将字符串写到显示内存的相应位置处
    //首先将显示指针display_ptr
    //指到屏幕第一行右端差4个字符处（每个字符需要2个字节）
    //因此减8
    display_ptr = ((char *)video_mem_start) + video_size_row -8;
    //然后循环复制字符串中的字符，并且每复制一个字符都空开一个属性字节
    while(*display_desc){
        *display_ptr++ = *display_desc++; //复制字符
        display_ptr++;  //空开属性字节位置
    }
    //初始化用于滚屏的变量（主要用于EGA/VGA）
    origin = video_mem_start; //滚屏起始显示内存地址
    scr_end = video_mem_start + video_num_lines * video_size_row;  //滚屏结束内存地址(774048)
    top = 0; //最顶行号
    bottom = video_num_lines; //最底行号(25)

    gotoxy(ORIG_X, ORIG_Y); //初始化光标位置x,y,和对应的内存位置pos
    set_trap_gate(0x21, &keyboard_interrupt); //设置键盘中断陷阱门
    outb_p(inb_p(0x21)&0xfd,0x21); //取消8259A中对键盘的中断屏蔽，允许IRQ1
    a = inb_p(0x61); //延迟读取键盘端口0x61（8255A端口PB）
    outb_p(a|0x80,0x61); //设置禁止键盘工作（位7置位）
    outb(a, 0x61); //再次允许键盘工作，用于复位键盘操作
}

