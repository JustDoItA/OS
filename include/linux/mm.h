#ifndef _MM_H
#define _MM_H

#define PAGE_SIZE 4096

//自己加的
extern void get_empty_page(unsigned long address);
//extern static int try_to_share(unsigned long address, struct task_struct * p);
extern unsigned long get_free_page(void);
extern void free_page(unsigned long addr);

#endif
