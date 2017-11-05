#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64
#define HZ 100

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS]


#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2

#include <linux/head.h>
#include <signal.h>
#include <linux/fs.h>
#include <linux/mm.h>

extern void trap_init(void);
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
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;



extern void sleep_on(struct task_struct **p);

extern void wake_up(struct task_struct ** p);


#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))

#define str(n) \
    __asm__("str %%ax\n\t" \
        "subl %2,%%eax\n\t" \
        "shrl $4,%%eax" \
            :"=a" (n) \
            :"a" (0),"i" (FIRST_TSS_ENTRY<<3))

#define switch_to(n){\
    struct {long a,b;} __tmp; \
    __asm__("cmpl %%ecx,current\n\t"\
            "je 1f\n\t"\
            "movw %%dx,%1\n\t" \
            "xchgl %%ecx,current\n\t" \
            "ljmp *%0\n\t" \
            "cmpl %%ecx,last_task_used_math\n\t" \
            "jne 1f\n\t" \
            "clts\n" \
            "1:" \
            ::"m" (*&__tmp.a),"m" (*&__tmp.b), \
            "d" (_TSS(n)),"c" ((long) task[n])); \
}

extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

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

//任务(进程)数据结构，或称为进程描述符
struct task_struct {
    // 任务运行状态(-1 不可运行， 0 可运行(就绪), >0 已停止)
    long state;
    // 任务运行时间计数(递减)(滴答数),运行时间片段
    long counter;
    // 运行优先数。 任务开始运行时 counter=priority,越大运行越长
    long priority;
    // 信号，是位图，每个比特位代表一种信号，信号值=位偏移值+1
    long signal;
    // 信号执行属性结构，对应信号将要执行的操作和标志信息
    struct sigaction sigaction[32];
    // 进程信号屏蔽码 (对应信号位图)
    long blocked;

    // 任务执行停止的退出码，其父进程会取
    int exit_code;
    // 代码段地址 代码长度(字节数) 代码长度+数据长度(字节数) 总长度(字节数)
    // 堆栈段地址
    unsigned long start_code,end_code,end_data,brk,start_start;
    // 进程标志号(进程号) 父进程号 父进程组号 会话号 会话首领
    long pid,father,pgrp,session,leader;
    // 用户标识号(用户id) 有效用户id 保存的用户id 组标识号(组id)
    unsigned short uid,euid,suid;
    // 组标识号(组id) 有效组id 保存的组id
    unsigned short gid,egid,sgid;
    // 报警定时值(滴答数)
    long alarm;
    // 用户态运行时间(滴答数) 系统态运行时间(滴答数) 子进程态用户态运行时间
    // 子进程系统态运行时间 进程开始运行时刻
    long utime,stime,cutime,cstime,start_time;
    // 是否使用协处理器
    unsigned short used_math;

    // 进程使用tty的子设备号。-1表示没有使用
    int tty;
    // 文件创建属性屏蔽位
    unsigned short umask;
    // 当前工作目录i节点结构
    struct m_inode * pwd;
    // 根目录i节点结构
    struct m_inode * root;
    // 执行文件i节点结构
    struct  m_inode * executable;
    // 执行时关闭文件局部位图标志(include/fcntl.h)
    unsigned long close_on_exec;
    // 进程使用的文件表结构
    struct file * file[NR_OPEN];

    // 本任务的局部表描述符 0:空 1：代码段cs  2：数据和堆栈段ds:ss
    struct desc_struct ldt[3];
    // 本进程任务状态段的信息结构
    struct tss_struct tss;

};

    extern void sleep_on(struct task_struct **p);

#define _get_base(addr)({                       \
            unsigned long __base;               \
            __asm__("movb %3,%%dh\n\t"          \
                    "movb %2,%%dl\n\t"          \
                    "shll $16,%%edx\n\t"        \
                    "movw %1,%%dx"              \
                    :"=d" (__base)              \
                    :"m" (*((addr)+2)),         \
                     "m" (*((addr)+4)),         \
                     "m" (*((addr)+7)));        \
            __base;})

#define get_base(ldt) _get_base(((char *)&(ldt)))

#define get_limit(segment)({      \
    unsigned long __limit; \
    __asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment));    \
    __limit;})
#endif
