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

extern int copy_page_tables(unsigned long from, unsigned long to, long size);
extern int free_page_tables(unsigned long form, unsigned long size);

extern void sched_init(void);

extern void trap_init(void);
extern void panic(const char * str);

//用于设置第一个任务表
        /*tss*/ 
        /*esp0=PAGE_SIZE+(long)&init_task 内核态堆栈指针初始化为页面的最后*/ 
        /*ss0=0x10 内核堆态堆栈的段选择符,指向系统数据段描述符,进程0的进程控制*/ 
        /*块和内核态堆栈都在system模块中*/ 
        /*cr3=(long)&pg_dir页目录表,其实linux0.11所有进程共享一个页目录*/ 
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
        /*ldt第0项为空*/ {0,0}, \
        /*ldt 代码段长640k,基地址0，G=1,D=1,DPL=3,P=1,TYPE=0x0a*/ {0x9f,0xc0fa00}, \
        /*ldt 数据段段长640k,基地址0，G=1,D=1,DPL=3,P=1,TYPE=0x02*/ {0x9f,0xc0f200}, \
        }, \
        {0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir, \
                 0,0,0,0,0,0,0,0, \
                 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
                 /*ldt表选择符指向gdt中LDT0处*/ _LDT(0),0x80000000, \
                 {} \
                 }, \
}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;



extern void interruptible_sleep_on(struct task_struct **p);
extern void add_timer(long jiffies, void(*fn)(void));
extern void sleep_on(struct task_struct **p);

extern void wake_up(struct task_struct ** p);


#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
//宏定义，计算在全局表中第n个任务的TSS描述符的索引号(选择符)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
//宏定义，加载第n个任务寄存器tr
#define ltr(n) __asm__("ltr %%ax" ::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))

#define str(n) \
    __asm__("str %%ax\n\t" \
        "subl %2,%%eax\n\t" \
        "shrl $4,%%eax" \
            :"=a" (n) \
            :"a" (0),"i" (FIRST_TSS_ENTRY<<3))

//将切换当前任务到nr,即n.首先检测任务n是不是当前任务
//如果是则什么也不做退出,如果我们切换到任务最近(上次运行)使用
//过协处理器的话，则还需要复位控制寄存器cr0中的TS标志

// 输入：%0-新TSS的偏移地址(*&_tmp.a)
// %1 存放新TSS的选择符(e&_tmp.b)
// dx 新任务n的选择符 ecx 新任务指针task[n]
// 其中临时数据结果__tmp中a的值是32位偏移值,b是新TSS的选择符
// 在任务切换时，a值没用(忽略).在判断新任务上次执行是否使用过
// 协处理器时，是通过将新任务状态段的地址与保存在last_task_used_math
// 变量中使用过协处理器任务状态段的地址进行比较而做出的
// dx = 48
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

#define _set_base(addr, base) \
    __asm__("movw %%dx,%0\n\t" \
        "rorl $16,%%edx\n\t" \
        "movb %%dl,%1\n\t" \
        "movb %%dh,%2" \
            ::"m" (*((addr)+2)), \
            "m" (*((addr)+4)), \
            "m" (*((addr)+7)), \
            "d" (base) \
        )//:"dx")

extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

typedef int (*fn_ptr)();

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
    long back_link; //保存前一个TSS段选择子，使用call指令切换寄存器时由CPU填写
    //这6个是固定不变的，用于提权，CPU切换栈的时候用
    long esp0; //保存0环栈指针
    long ss0; //保存0环栈选择子
    long esp1;//保存1环栈指针
    long ss1; //保存1环段选择子
    long esp2;//用于保存2环栈指针
    long ss2; //用于保存2环栈指针
    //下面这些都是用来做切换寄存器值用的，切换的时候由
    //CPU自动填写
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
    struct file * filp[NR_OPEN];

    // 本任务的局部表描述符 0:空 1：代码段cs  2：数据和堆栈段ds:ss
    struct desc_struct ldt[3];
    // 本进程任务状态段的信息结构
    struct tss_struct tss;

};

    extern void sleep_on(struct task_struct **p);

#define set_base(ldt,base) _set_base(((char *)&(ldt)), base)

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

//取段选择符segment的段长值
//%0 存放段长值(字节数) %1 段选择符segment
//lsll加载段界限的指令，把segment段描述符中的段界限装入某个寄存器(这个寄存器与
//__limit结合),函数返回__limit加1，即段长
#define get_limit(segment)({      \
    unsigned long __limit; \
    __asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment));    \
    __limit;})
#endif
