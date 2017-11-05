/*
 *@lpf
 *Sat Oct 14 22:37:43 PDT 2017
 */

#include <linux/sched.h>
#include <linux/kernel.h>

extern void write_verify(unsigned long address);//import this

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
