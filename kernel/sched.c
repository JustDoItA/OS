#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/mm.h>
#include <signal.h>

#define _S(nr) (1<<((nr)-1))
#define _BLOCKABLE (~(_S(SIGKILL)) | _S(SIGSTOP))

void show_task(int nr, struct task_struct * p){
    int i,j = 4096-sizeof(struct task_struct);

    printk("%d: pid=%d, state=%d ", nr, p->pid,p->state);
    i = 0;
    while(i<j && !((char *)(p+1))[i]){
        i++;
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
    struct task_struct **p;

    for (p = &LAST_TASK ;p > &FIRST_TASK; --p){
        if(*p){
            if ((*p)->alarm && (*p)->alarm < jiffies){
                (*p)->signal |= (1<<(SIGALRM-1));
                (*p)->alarm = 0;
            }
            if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
                (*p)->state==TASK_INTERRUPTIBLE){
                (*p)->state=TASK_RUNNING;
            }
        }
    }

    while(1){
        c = -1;
        next = 0;
        i = NR_TASKS;
        p = &task[NR_TASKS];
        while(--i){
            if (!*--p){
                continue;
            }
            if((*p)->state == TASK_RUNNING && (*p)->counter > c){
                c = (*p)->counter, next = i;
            }
            if (c){
                break;
            }
            for (p = &LAST_TASK; p > &FIRST_TASK; --p){
                if (*p){
                    (*p)->counter = ((*p)->counter >> 1)+(*p)->priority;
                }
            }
        }
        switch_to(next);
    }
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

void wake_up(struct task_struct **p){
    if (p && *p){
        (**p).state=0;
        *p=NULL;
    }
}
