/*
  @lpf
  Fri Sep  8 20:19:11 PDT 2017
 */
#include <linux/kernel.h>
#include <linux/sched.h>

void sys_sync(void);

void panic (const char *s){

    printk("Kernel panic: %s\n\r", s);
    if (current == task[0]){
        printk("In swapper task - not syncing\n\r");
    }else{
        sys_sync();
    }
    for(;;);

}
