/*
@lpf
Fri Sep  8 21:21:48 PDT 2017
 */
#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <linux/head.h>

#define move_to_user_mode()     \
    __asm__ ("movl %%esp,%%eax\n\t" \
        "pushl $0x17\n\t"  \
        "pushl %%eax\n\t" \
        "pushfl\n\t" \
        "pushl $0x0f\n\t" \
        "pushl $1f\n\t" \
        "iret\n" \
        "1:\tmovl $0x17, %%eax\n\t" \
        "movw %%ax, %%ds\n\t" \
        "movw %%ax, %%es\n\t" \
        "movw %%ax, %%fs\n\t" \
        "movw %%ax, %%gs" \
        :::"ax")

#define cli() __asm__ ("cli"::)
#define sti() __asm__ ("sti"::)

#define _set_gate(gate_addr, type, dpl, addr)        \
    __asm__ ("movw %%dx,%%ax\n\t" \
        "movw %0,%%dx\n\t" \
        "movl %%eax,%1\n\t" \
        "movl %%edx,%2" \
        :\
        : "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
        "o" (*((char *) (gate_addr))),              \
        "o" (* (4+(char *) (gate_addr))),           \
        "d" ((char *) (addr)),"a" (0x00080000))

//设置中断门函数
//参数： n-中断号； addr - 中断程序偏移地址
// &idt[n]对应中断号在中断描述符表中的偏移值；
// 中断描述符类型14，特权级0
#define set_intr_gate(n, addr)                       \
    _set_gate(&idt[n], 14, 0, addr)

#define set_trap_gate(n, addr)                       \
    _set_gate(&idt[n], 15, 0, addr)

#define set_system_gate(n, addr)                     \
    _set_gate(&idt[n], 15, 3, addr)

//在全局表中设置任务状态段/局部描述符
//参数：n 在全局表中描述符项n所对于的地址；addr 状态段/局部表所在的内存的基地址
//type 描述符中的标志类型字节
// %0 - eax(地址addr);%1 描述符项n的地址 %2 描述符项n的地址偏移2处
// %3 描述符项n的地址偏移4处 %4 描述符项n的地址偏移5处
// %5 描述符项n的地址6处 %6 描述符n的地址偏移7处

  //将TSS长度放入描述符长度域(第0-1字节)
  //将基地址的低字节放入描述符第2-3字节
  //将基地址高字移入ax中
  //将基地址高字中低字节移入描述符第4字节
  //将标志类型字节移入描述符的5字节
  //描述符的第6字节置0
  //将基地址高字中高字节移入描述符第7字节
  //eax 清零
#define _set_tssldt_desc(n,addr,type) \
    __asm__("movw $104,%1\n\t" \
            "movw %%ax,%2\n\t" \
            "rorl $16,%%eax\n\t" \
            "movb %%al,%3\n\t" \
            "movb $" type ",%4\n\t" \
            "movb $0x00,%5\n\t"     \
            "movb %%ah,%6\n\t"      \
            "rorl $16,%%eax" \
            ::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
            "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
        )

//在全局表中设置任务状态段描述符
//n-是该描述符的指针；addr 是描述符中的基地址值，任务状态段描述的类型是0x89
#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
#endif
