/*
 *@lpf
 *Sat Oct 14 22:37:43 PDT 2017
 */

#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);//import this

long last_pid = 0;

void verify_area(void * addr,int size){
    unsigned long start;

    start = (unsigned long) addr;
    size += start & 0xfff;
    start &= 0xfffff000;
    start =+ get_base(current->ldt[2]);
    while(size>0){
        size -= 4096;
        write_verify(start);
        start += 4096;
    }

}

//设置新任务的代码和数据段基址,限长并复制页表
//nr为新任务号 p是新任务的数据结构的指针
int copy_mem(int nr, struct task_struct * p){
    unsigned long old_data_base, new_data_base, data_limit;
    unsigned long old_code_base, new_code_base, code_limit;

    code_limit = get_limit(0x0f);//取局部描述符表中代码段描述符项中段限长
    data_limit = get_limit(0x17);//取局部描述符表中数据段描述符项中段限长
    old_code_base = get_base(current -> ldt[1]);//取原代码段基址
    old_data_base = get_base(current -> ldt[2]);//取原数据段基址
    if(old_data_base != old_code_base){ //0.11版本不纸质代码段和数据段分立的情况
        panic("We do not support serparate I&D");
    }
    if(data_limit < code_limit){ //如果数据段长度<代码段长度页不对
        panic("Bad data_limit");
    }
    new_data_base = new_code_base = nr * 0x4000000; //新基址=任务号*64MB(任务大小)
    p -> start_code = new_code_base;
    set_base(p -> ldt[1], new_code_base);//设置代码段描述符中基址域
    set_base(p -> ldt[2], new_data_base);//设置数据段描述符中基址域
        //复制代码和数据段
    if(copy_page_tables(old_data_base, new_data_base, data_limit)){
        //如果出差则释放申请的内存
        free_page_tables(new_data_base, data_limit);
        return -ENOMEM;
    }
    return 0;
}

//fork子程序，复制系统进程信息(task[n])并且设置必要的寄存器
//复制进程
int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
                 long ebx, long ecx, long edx, long fs, long es, long ds,
                 long eip, long cs, long eflags, long esp, long ss){
    struct task_struct *p;
    int i;
    struct file *f;

    //为新任务数据结果分配内存
    p = (struct task_struct *) get_free_page();
    if(!p){//如果内存分配出错,则返回错误码并退出
        return -EAGAIN;
    }
    //nr 为任务号,由前面find_emtpy_process返回
    task[nr] = p;
    //注意 这样不会复制超级用户的堆栈(只复制当前进程内容)
    *p = *current;
    p -> state = TASK_UNINTERRUPTIBLE; //将新进程的状态设置为不可中断等待状态
    p -> pid = last_pid;//新进程号，由前面调用find_empty_process得到
    p -> father = last_pid; //设置父进程号
    p -> counter = p -> priority;
    p -> signal = 0;    //信号位图置0
    p -> alarm = 0;
    p -> leader = 0;
    //进程的领导权是不能继承的
    p -> utime = p -> stime = 0; //初始化用户态的时间和核心态时间
    p -> cutime = p -> cstime = 0;//初始化子进程用户态和核心态时间
    p -> start_time = jiffies; //当前滴答数时间
    //以下设置任务状态段TSS所需要的数据
    p -> tss.back_link = 0;
    p -> tss.esp0 = PAGE_SIZE+(long) p; //堆栈指针(由于任务给结构p分配了1页)
    //新内存，所以此时esp0正好指向该页顶端)
    p -> tss.ss0 = 0x10;    //堆栈段选择符(内核数据段)
    p -> tss.eip = eip;     //指令代码指针
    p -> tss.eflags = eflags; //标志寄存器
    p -> tss.eax = 0;
    p -> tss.ecx = ecx;
    p -> tss.edx = edx;
    p -> tss.esp = esp;
    p -> tss.ebp = ebp;
    p -> tss.esi = esi;
    p -> tss.edi = edi;
    p -> tss.es = es & 0xffff; //段寄存器仅16位有效
    p -> tss.cs = cs & 0xffff;
    p -> tss.ss = ss & 0xffff;
    p -> tss.ds = ds & 0xffff;
    p -> tss.fs = fs & 0xffff;
    p -> tss.gs = gs & 0xffff;
    p -> tss.ldt = _LDT(nr); //改新任务nr的局部描述符表选择符(LDT的描述符在GDT中)
    p -> tss.trace_bitmap = 0x80000000;
    //如果当前任务使用了协处理器，就保存其上下文
    if(last_task_used_math == current){
        __asm__("clts ; fnsave %0"::"m" (p -> tss.i387));
    }
    //设置新任务的代码和数据段地址，限长并复制页表，如果出错(返回值不是0),则复位
    //则复位任务数组中的相应项，并释放为该新任务分配的内存页
    if(copy_mem(nr, p)){
        task[nr] = NULL;
        free_page((long) p);
        return -EAGAIN;
    }
    //如果父进程中有文件是打开的，则将对应文件的打开次数加1
    for(i = 0; i < NR_OPEN; i++){
        if((f = p->filp[i])){
            f -> f_count++;
        }
    }
    //当前进程(父进程)的pwd，root，executable引用次数均增1
    if(current -> pwd){
        current -> pwd -> i_count++;
    }
    if(current -> root){
        current -> root -> i_count++;
    }
    if(current -> executable){
        current -> executable -> i_count++;
    }
    //在GDT中设置新任务的TSS和LDT描述项，数据从task结构中取
    //在任务切换时，任务寄存器tr由CPU自动加载
    set_tss_desc(gdt +(nr << 1) + FIRST_TSS_ENTRY, &(p -> tss));
    set_ldt_desc(gdt +(nr << 1) + FIRST_LDT_ENTRY, &(p -> tss));
    //最后再将新任务设置为可运行状态 以防万一？？？？
    p -> state = TASK_RUNNING;
    return last_pid;

}

//为新进程取得不重复的进程号lost_pid,并返回在任务数组中的任务号(数组index)
int find_empty_process(void){
    int i;

repeat:
    if((++last_pid)<0){
        last_pid = 1;
    }
    for(i = 0; i < NR_TASKS; i++){
        if(task[i] && task[i]->pid == last_pid){
            goto repeat;
        }
    }
    for(i = 1; i < NR_TASKS; i++){
        if(!task[i]){
            return i;
        }
    }
    return -EAGAIN;
}
