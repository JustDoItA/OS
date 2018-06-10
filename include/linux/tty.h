/*
   @lpf
   Sun Nov  5 01:16:03 PST 2017
 */

#ifndef _TTY_H
#define _TTY_H

#include <termios.h> //终端输入输出函数头文件，主要定义控制异步通信口的终端接口

#define TTY_BUF_SIZE 1024

//tty 等待队列数据结构
struct tty_queue {
    unsigned long data;    //等待队列缓冲区当前数据指针字符数
    //对于串口终端，则存放串行端口地址
    unsigned long head;     //缓冲区中的数据头指针
    unsigned long tail;     //缓冲区中数据尾指针
    struct task_struct * proc_list; //等待进程列表
    char buf[TTY_BUF_SIZE];    //队列缓冲区
};

//a缓冲区指针前移1字节，并循环
//#define INC(a) ((a)=((a)+1))
#define INC(a) ((a) = ((a)+1))
#define DEC(a) ((a) = ((a)-1) & (TTY_BUF_SIZE-1))

//清空指定队列缓冲区
#define EMPTY(a) ((a).head == (a).tail)
//缓冲区还可以存放字符的长度(空闲区长度)
#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))
#define LAST(a) ((a).buf[(TTY_BUF_SIZE-1)&((a).head-1)])
#define FULL(a) (!LEFT(a))
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE-1))
//从queue队列项缓冲区中取一字符(从tail处，并且tail+=1)
#define GETCH(queue,c) \
    (void)({c=(queue).buf[(queue).tail];INC((queue).tail);})
#define PUTCH(c,queue) \
    (void)({(queue).buf[(queue).head]=(c);INC((queue).head);})

#define INTR_CHAR(tty) ((tty)->termios.c_cc[VINTR])
#define QUIT_CHAR(tty) ((tty)->termios.c_cc[VQUIT])
#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])
#define KILL_CHAR(tty) ((tty)->termios.c_cc[VKILL])
#define EOF_CHAR(tty) ((tty)->termios.c_cc[VEOF])
#define START_CHAR(tty) ((tty)->termios.c_cc[VSTART])
#define STOP_CHAR(tty) ((tty)->termios.c_cc[VSTOP])

//tty数据结构
struct tty_struct {
    struct termios termios;             //终端io属性和控制字符数据结构
    int pgrp;                           //所属进程组
    int stopped;                        //停止标志
    void (*write)(struct tty_struct * tty);//tty 写函数指针
    struct tty_queue read_q;            //tty 读队列
    struct tty_queue write_q;           //tty 写队列
    struct tty_queue secondary;        //tty辅助队列（存放规范模式字符序列）
                                        //可称为规范（熟）模式队列
};

extern struct tty_struct tty_table[];

#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

void rs_init(void);
void con_init(void);
void tty_init(void);
void tty_intr(struct tty_struct * tty, int mask);

void rs_write(struct tty_struct *tty);
void con_write(struct tty_struct *tty);
void copy_to_cooked(struct tty_struct *tty);

#endif
