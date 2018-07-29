#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/fdreg.h>

#include <asm/system.h>
#include <asm/io.h>

#include <linux/mm.h>
#include <signal.h>

#define _S(nr) (1<<((nr)-1))
#define _BLOCKABLE (~(_S(SIGKILL)) | _S(SIGSTOP))

void show_task(int nr, struct task_struct * p){
    int i,j = 4096-sizeof(struct task_struct);

    printk("%d: pid=%d, state=%d ", nr, p->pid,p->state);
    i = 0;
    while(i<j && !((char *)(p+1))[i]){i++;
    }
    printk("%d (of %d) chars free in kernel stack \n\t",i,j);
}

void show_stat(void){
    int i;
    for(i = 0;i < NR_TASKS; i++){
        if(task[i]){
            show_task(i,task[i]);
        }
    }
}

#define LATCH (1193180/HZ)


extern int timer_interrupt(void);
extern int system_call(void);

extern void schedule(void);

long startup_time = 0;

union task_union{
    struct task_struct task;
    char stack[PAGE_SIZE];
};

static union task_union init_task = {INIT_TASK,};

long volatile jiffies = 0;

struct task_struct *current = &(init_task.task);
struct task_struct *last_task_used_math = NULL;

struct task_struct * task[NR_TASKS] = {&(init_task.task)};

long user_stack [ PAGE_SIZE>>2 ];

//long stack_start =0x00107c00;
struct {
    long * a;
    short b;
} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10};
//} stack_start = { 0x7c00 , 0x0010};

void math_state_restore(){
    if(last_task_used_math == current){
        return;
    }
    __asm__("fwait");
    if(last_task_used_math){
        __asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
    }
    last_task_used_math = current;
    if(current->used_math){
        __asm__("fnsave %0"::"m" (current->tss.i387));
    }else{
        __asm__("fninit"::);
        current -> used_math = 1;
    }
}

void schedule(void){
    int i,next,c;
    struct task_struct **p; //任务结构指针的指针
    //检测alarm(进程的报警定时值),唤醒任何以已得到引号的可中断任务

    //从任务数组中最后一个任务开始检测alarm
    for (p = &LAST_TASK ;p > &FIRST_TASK; --p){
        if(*p){
            //如果任务的alarm时间已经(alarm<jiffies),则在信号位图中置SIGALRM信号,
            //然后清alarm.jiffies 是系统从开机开始算起的滴答数(10ms/滴答)
            if ((*p)->alarm && (*p)->alarm < jiffies){
                (*p)->signal |= (1<<(SIGALRM-1));
                (*p)->alarm = 0;
            }
            //如果信号位图中被阻塞的信号外还有其它信号，并且任务属于可中断状态，
            //则任务置为就绪状态
            //其中～(_BLOCKABLE & (*p)->blocked)用于忽略被阻塞信号，但SIGKILL和SIGSTOP 不能被阻塞
            if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
                (*p)->state==TASK_INTERRUPTIBLE){
                (*p)->state=TASK_RUNNING; //置为就绪(可执行)状态
            }
        }
    }

    while(1){
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &task[NR_TASKS];
        //这段代码也是从任务数组中的最后一个任务开始循环处理，并跳过
        //不含任务的数组槽，比较每个就绪状态任务的counter(任务运行时间的递减滴答计数)
        //值，哪个大，运行时间还不长，next就指向那个任务号
        while(--i){
            if (!*--p){
                continue;
            }
            if((*p)->state == TASK_RUNNING && (*p)->counter > c){
                c = (*p)->counter, next = i;
            }
            //如果比较得出有counter值大于0的结果，则退出124行开始的循环
            if (c){
                break;
            }
            //否则就根据每个任务的优选权值，更新每个任务的counter值然后重新
            //比较
            //counter值的计算方式为counter = counter/2 + priority
            for (p = &LAST_TASK; p > &FIRST_TASK; --p){
                if (*p){
                    (*p)->counter = ((*p)->counter >> 1)+(*p)->priority;
                }
            }
        }
        switch_to(next);//切换到任务号为next的任务，并运行
    }
}

int sys_pause(void){
    current->state = TASK_INTERRUPTIBLE;
    schedule();
    return 0;
}


void sleep_on(struct task_struct **p){
    struct task_struct *tmp;

    if (!p){
        return ;
    }
    if (current == &(init_task.task)){
        panic("task[0] trying to sleep");
    }
    tmp = *p;
    *p = current;
    current->state = TASK_UNINTERRUPTIBLE;
    schedule();
    if (tmp){
        tmp->state=0;
    }
}

void interruptible_sleep_on(struct task_struct **p){
    struct task_struct * tmp;

    if(!p){
        return;
    }
    if(current == &(init_task.task)){
        //panic("task[0] trying to sleep");
    }
    tmp = *p;
    *p = current;
repeat: current->state = TASK_INTERRUPTIBLE;
    schedule();
    if(*p && *p != current){
        (**p).state=0;
        goto repeat;
    }
    *p = NULL;
    if(tmp){
        tmp->state=0;
    }
}

//唤醒指定任务*p
void wake_up(struct task_struct **p){
    if (p && *p){
        (**p).state=0; //置为就绪(可运行)状态
        *p=NULL;
    }
}

static struct task_struct * wait_motor[4] = {NULL,NULL,NULL,NULL};
static int mon_timer[4] = {0,0,0,0};
static int moff_timer[4] = {0,0,0,0};
unsigned char current_DOR = 0x0C;

int ticks_to_floppy_on(unsigned int nr){
    extern unsigned char selected;
    unsigned char mask = 0x01 << nr;

    if(nr > 3){
        //panic("floppy_on: nr>3");
    }
    moff_timer[nr]=10000;
    cli();
    mask |= current_DOR;
    if(!selected){
        mask &= 0xFC;
        mask |= nr;
    }
    if(mask != current_DOR){
        outb(mask,FD_DOR);

        if((mask ^ current_DOR) & 0xf0){
            mon_timer[nr] = HZ/2;
        }else if(mon_timer[nr] < 2){
            mon_timer[nr] = 2;
        }
        current_DOR = mask;
    }
    sti();
    return mon_timer[nr];
}

void floppy_on(unsigned int nr){
    cli();
    while(ticks_to_floppy_on(nr)){
        sleep_on(nr+wait_motor);
    }
    sti();
}

void floppy_off(unsigned int nr){
    moff_timer[nr] = 3*HZ;
}

void do_floppy_timer(){
    int i;
    unsigned char mask = 0x10;

    for(i=0; i<4; i++,mask <<=1){
        if(!(mask & current_DOR)){
            continue;
        }
        if(mon_timer[i]){
            if(!--mon_timer[i]){
                wake_up(i+wait_motor);
            }
        }else if(!moff_timer[i]){
            current_DOR &= ~mask;
            outb(current_DOR, FD_DOR);
        }else {
            moff_timer[i]--;
        }
    }
}

#define TIME_REQUESTS 64

static struct timer_list {
    long jiffies;
    void (*fn)();
    struct timer_list * next;
} timer_list[TIME_REQUESTS], *next_timer=NULL;

void add_timer(long jiffies, void (*fn)(void)){
    struct timer_list * p;

    if(!fn){
        return;
    }
    cli();
    if(jiffies <= 0){
        (fn)();
    }else {
        for(p = timer_list; p < timer_list +TIME_REQUESTS; p++){
            if(!p->fn){
                break;
            }
        }
    if(p >= timer_list + TIME_REQUESTS){
        //panic("No more time requests free");
    }
    p->fn = fn;
    p->jiffies = jiffies;
    p->next = next_timer;
    next_timer = p;
    while(p->next && p->next->jiffies < p->jiffies){
        p->jiffies -= p->next->jiffies;
        fn = p->fn;
        p->fn = p->next->fn;
        p->next->fn = fn;
        jiffies = p->jiffies;
        p->jiffies = p->next->jiffies;
        p->next->jiffies = jiffies;
        p = p->next;
    }
    }
    sti();
}

//时钟中断c处理函数，在kernel/system_call.s中的timer_interrupt
//被调用，对于一个进程由于执行时间片用完时，则进行任务切换并
//执行一个计时更新工作
void do_timer(long cpl){
    extern int beepcount;
    extern void sysbeepstop(void);
    if(beepcount){
        if (!--beepcount){
            sysbeepstop();
        }
    }
    if(cpl){
        current->utime++;
    }else{
        current->stime++;
    }

    if(next_timer){
        next_timer->jiffies--;
        while (next_timer && next_timer->jiffies <= 0 ){
            void (*fn)(void);
            fn = next_timer->fn;
            next_timer->fn = NULL;
            next_timer = next_timer->next;
            (fn)();
        }
    }
    if (current_DOR & 0xf0){
        do_floppy_timer();
    }
    if((--current->counter)) return;
    current->counter=0;
    if(!cpl) return;
    schedule();
}

int sys_alarm(){
    return 0;
}

int sys_getpid(){
    return 0;
}

int sys_getuid(){
    return 0;
}

//调度程序的初始化子程序
void sched_init(){
    int i;
    struct desc_struct *p; //描述符表结构指针 每项8字节共256项

    if (sizeof(struct sigaction) != 16){//sigaction 是存放有关信号状态的结构
        //panic("Struct sigaction MUST be 16 bytes");
    }
    //设置初始话任务(任务0)的任务状态段描述符和局部数据表描述符
    //(include/asm/system.h) FIRST_TSS_ENTRY 4,表示在描述符表的索引是4
    //因为gdt是desc_struct类型(8字节),刚好是一个描述符的长度，所以这里的
    //gdt+4可以理解为gdt[4],刚好对应的是TSS0
    set_tss_desc(gdt+FIRST_TSS_ENTRY, &(init_task.task.tss));
    set_ldt_desc(gdt+FIRST_LDT_ENTRY, &(init_task.task.ldt));
    //清任务数组和描述符表项(注意i=1开始，所以初始化任务的描述符还在)
    //p指向GDT表0号任务的下一个位置,即GDT表的第6项
    p = gdt+2+FIRST_TSS_ENTRY;
    for(i=1 ;i<NR_TASKS; i++){
        task[i] = NULL;
        //重复两次是因为每个进程对于一个LDT与一个TSS
        p->a = p->b=0;
        p++;
        p->a = p->b=0;
        p++;
    }

    //清除标志寄存器中的位NT，
    //NT标志位用于控制程序的递归调用，当NT置位时，那么当前前中断任务执行iret
    //指令时就会引起任务切换，NT指出TSS中的black_link字段是否有效
    __asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");//复位NT标志
    ltr(0); //将任务0的TSS加载到任务寄存器tr
    lldt(0); //将局部描述符表加载到局部描述符表寄存器
    //注意：将GDT中相应LDT描述符加载到ldtr，只明确加载这一次，以后新任务LDT的加
    //载，是CUP根据TSS中的LDT项自动加载
    //下代码用于初始化8253定时器
    outb_p(0x36,0x43);
    outb_p(LATCH & 0xff, 0x40); //LSB 定时值低字节
    outb(LATCH >> 8, 0x40);     //MSB 定时值高字节
    //设置时钟中断处理程序句柄(设置时钟中断门)
    set_intr_gate(0x20, &timer_interrupt);
    //修改中断控制器屏蔽码，允许时钟中断
    outb(inb_p(0x21)&~0x01,0x21);
    //设置系统调用中断门
    set_system_gate(0x80, &system_call);
}
