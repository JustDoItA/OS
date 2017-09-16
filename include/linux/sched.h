#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS]


#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2

#include <linux/head.h>
#include <signal.h>
#include <linux/fs.h>
#include <linux/mm.h>

extern void panic(const char * str);


#define INIT_TASK \
    /*state etc*/    { 0,15,15, \
        /*signals*/  0,{{},},0, \
        /*ec,brk*/   0,0,0,0,0,0, \
        /*pid etc*/  0,-1,0,0,0, \
        /*uid etc*/  0,0,0,0,0,0, \
        /*alarm*/    0,0,0,0,0,0,  \
        /*math*/     0, \
        /*fs info*/   -1,0022,NULL,NULL,NULL,0, \
        /*filp*/      {NULL,}, \
        {\
        {0,0}, \
        /*ldt*/  {0x9f,0xc0fa00}, \
                 {0x9f,0xc0f200}, \
        }, \
        /*tss*/ {0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir, \
                 0,0,0,0,0,0,0,0, \
                 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
                 _LDT(0),0x80000000, \
                 {} \
                 }, \
}

extern struct task_struct *task[NR_TASKS];

extern struct task_struct *current;
extern long volatile jiffies;



extern void sleep_on(struct task_struct ** p);



#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))

#define switch_to(n){\
    struct {long a,b;} __tmp; \
    __asm__("cmpl %%ecx,current\n\t"\
            "je 1f\n\t"\
            "movw %%dx,%1\n\t" \
            "xchgl %%ecx,current\n\t" \
            "ljmp %0\n\t" \
            "cmpl %%ecx,last_task_used_math\n\t" \
            "jne 1f\n\t" \
            "clts\n" \
            "1:" \
            ::"m" (*&__tmp.a),"m" (*&__tmp.b), \
            "d" (_TSS(n)),"c" ((long) task[n])); \
}

extern long startup_time;

struct i387_struct {
    long cwd;
    long swd;
    long twd;
    long fip;
    long fcs;
    long foo;
    long fos;
    long st_space[32];
};

struct tss_struct {
    long back_link;
    long esp0;
    long ss0;
    long esp1;
    long ss1;
    long esp2;
    long ss2;
    long cr3;
    long eip;
    long eflags;
    long eax,ecx,edx,ebx;
    long esp;
    long ebp;
    long esi;
    long edi;
    long es;
    long cs;
    long ss;
    long ds;
    long fs;
    long gs;
    long ldt;
    long trace_bitmap;
    struct i387_struct i387;
};

struct task_struct {
    long state; //-1不可运行，0可运行，>0停止
    long counter;
    long priority;
    long signal;
    struct sigaction sigaction[32];
    long blocked;

    int exit_code;
    unsigned long start_code,end_code,end_data,brk,start_start;
    long pid,father,pgrp,session,leader;
    unsigned short uid,euid,suid;
    unsigned short gid,egid,sgid;
    long alarm;
    long utime,stime,cutime,cstime,start_time;
    unsigned short used_math;

    int tty;
    unsigned short umask;
    struct m_inode * pwd;
    struct m_inode * root;
    struct  m_indoe * executable;
    unsigned long close_on_exec;
    struct file * file[NR_OPEN];

    struct desc_struct ldt[3];
    struct tss_struct tss;

};

    extern void wake_up(struct task_struct **p);
#endif
