/*
  @lpf
  Sun Nov  5 01:11:53 PST 2017
*/

#include <ctype.h>
#include <linux/tty.h>
#include <termios.h>
#include <signal.h>
#include <asm/system.h>


#define INTMASK (1<<(SIGINT-1))
#define QUITMASK (1<<(SIGQUIT-1))

#include <linux/sched.h>

#define _L_FLAG(tty,f) ((tty)->termios.c_lflag & f)
#define _I_FLAG(tty,f) ((tty)->termios.c_iflag & f)

#define L_CANON(tty) _L_FLAG((tty),ICANON)
#define L_ISIG(tty) _L_FLAG((tty),ISIG)

#define L_ECHO(tty) _L_FLAG((tty),ECHO)


#define L_ECHOCTL(tty) _L_FLAG((tty),ECHOCTL)

#define I_UCLC(tty) _I_FLAG((tty),IUCLC)
#define I_NLCR(tty) _I_FLAG((tty),INLCR)
#define I_CRNL(tty) _I_FLAG((tty),ICRNL)
#define I_NOCR(tty) _I_FLAG((tty),IGNCR)

struct tty_struct tty_table[] = {
    {
        {ICRNL,
         OPOST|ONLCR,
         0,
         ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
         0,
         INIT_C_CC},
         0,
         0,
         con_write,
        {0,0,0,0,""},
        {0,0,0,0,""},
        {0,0,0,0,""}
    },{
        {0,
         0,
        B2400 | CS8,
        0,
        0,
        INIT_C_CC},
        0,
        0,
        rs_write,
        {0x3f8,0,0,0,""},
        {0x3f8,0,0,0,""},
        {0,0,0,0,""}
    },{
        {0,
         0,
         B2400 | CS8,
         0,
         0,
         INIT_C_CC},
        0,
        0,
        rs_write,
        {0x2f8,0,0,0,""},
        {0x2f8,0,0,0,""},
        {0,0,0,0,""}
    }
};

struct tty_queue *table_list[] ={
    &tty_table[0].read_q, &tty_table[0].write_q,
    &tty_table[1].read_q, &tty_table[1].write_q,
    &tty_table[2].read_q, &tty_table[2].write_q,
};

void tty_init(){
    rs_init();
    con_init();
}

void tty_intr(struct tty_struct *tty, int mask){
    int i;

    if(tty->pgrp <= 0){
        return; 
    }
    for (i=0;i<NR_TASKS;i++){
        if(task[i] && task[i]->pgrp==tty->pgrp){
                task[i]->signal |= mask; 
        }
    }
}

static void sleep_if_empty(struct tty_queue * queue){
    cli();
    while(!current->signal && EMPTY(*queue)){
        interruptible_sleep_on(&queue->proc_list);
    }
    sti();
}


void wait_for_keypress(void){
    sleep_if_empty(&tty_table[0].secondary);
}

void copy_to_cooked(struct tty_struct *tty){

    signed char c;

    while (!EMPTY(tty->read_q) && !FULL(tty->secondary)){
        GETCH(tty->read_q, c);
        if (c == 13){
            if (I_CRNL(tty)) {
                c = 10;
            }else if(I_NOCR(tty)){
                continue;
            }else{
                ;
            }
        } else if (c==10 && I_NLCR(tty)){
            c=13;
        }
        if (I_UCLC(tty)){
            c=tolower(c);
        }
        if (L_CANON(tty)){
            if (c==KILL_CHAR(tty)) {
                while(!(EMPTY(tty->secondary) ||
                        (c=LAST(tty->secondary))==10 ||
                        c==EOF_CHAR(tty))){
                    if(L_ECHO(tty)){
                        if (c<32){
                        PUTCH(127,tty->write_q);
                        }
                    PUTCH(127,tty->write_q);
                    tty->write(tty);
                    }
                    DEC(tty->secondary.head);
                }
            continue;
            }
        if(c==ERASE_CHAR(tty)){
            if(EMPTY(tty->secondary) ||
               (c=LAST(tty->secondary))==10 ||
               c==EOF_CHAR(tty)){
                continue;
            }
            if(L_ECHO(tty)){
                if(c<32){
                    PUTCH(127,tty->write_q);
                }
                PUTCH(127,tty->write_q);
                tty->write(tty);
            }
            DEC(tty->secondary.head);
            continue;
        }
        if(c==STOP_CHAR(tty)){
            tty->stopped=1;
            continue;
        }
        if(c==START_CHAR(tty)){
            tty->stopped=0;
            continue;
        }
        }
        if(L_ISIG(tty)){
            if (c==INTR_CHAR(tty)){
                tty_intr(tty,INTMASK);
                continue;
            }
            if(c==QUIT_CHAR(tty)){
                tty_intr(tty,QUITMASK);
                continue;
            }
        }
        if(c==10 || c==EOF_CHAR(tty)){
            tty->secondary.data++;
        }
        if(L_ECHO(tty)){
            if(c==10){
                PUTCH(10,tty->write_q);
                PUTCH(13,tty->write_q);
            } else if(c<32){
                if (L_ECHOCTL(tty)){
                    PUTCH('^',tty->write_q);
                    PUTCH(c+64,tty->write_q);
                }
            } else {
                PUTCH(c,tty->write_q);
            }
            tty->write(tty);
        }
        PUTCH(c,tty->secondary);
    }
    //wake_up(&tty->secondary.proc_list);
     //   rs_init();
}

/*tty中断处理调用函数-执行tty中断处理。
 * 参数: tty - 指定的tty终端号（0,1,或2）。
 * 将指定的tty终端队列缓冲区中的字符复制成规范模式字符并放在
 * 辅助队列读中。
 * 在串口读字符中断rs_io.s和键盘中断keyboard.S中调用
 */

void do_tty_interrupt(int tty){
    copy_to_cooked(tty_table + tty);
}

//字符设备初始化函数。空 为以后扩展做准备
void chr_dev_init(){

}
