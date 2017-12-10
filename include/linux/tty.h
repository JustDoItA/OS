/*
   @lpf
   Sun Nov  5 01:16:03 PST 2017
 */

#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

#define TTY_BUF_SIZE 1024

struct tty_queue {
    unsigned long data;
    unsigned long head;
    unsigned long tail;
    struct task_struct * proc_list;
    char buf[TTY_BUF_SIZE];
};

#define INC(a) ((a) = ((a)+1) & (TTY_BUF_SIZE))

#define EMPTY(a) ((a).head == (a).tail)
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE))
#define GETCH(queue,c) \
    (void)({c=(queue).buf[(queue).tail];INC((queue).tail);})


#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])

struct tty_struct {
    struct termios termios;
    int pgrp;
    int stopped;
    void (*write)(struct tty_struct * tty);
    struct tty_queue read_q;
    struct tty_queue write_q;
    struct tty_queue secondary;
};

extern struct tty_struct tty_table[];

#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

void rs_init(void);
void con_init(void);
void tty_init(void);

void rs_write(struct tty_struct *tty);
void con_write(struct tty_struct *tty);

#endif
