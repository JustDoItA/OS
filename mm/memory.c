/*
  @lpf
  Sun Sep 17 01:50:03 PDT 2017
*/

#include <signal.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>

void do_exit(long code);
//后期自己添加
static int try_to_share(unsigned long address, struct task_struct * p);

static inline void oom(void){
    printk("out of memory\n\r");
    do_exit(SIGSEGV);
}

#define invalidate() \
    __asm__("movl %%eax,%%cr3"::"a"(0))

#define LOW_MEM 0x100000                        // 内存低端(1M)
#define PAGING_MEMORY (15 * 1024 * 1024)        // 分页内存15M(内存区最多15M)
#define PAGING_PAGES (PAGING_MEMORY >> 12)      // 分页后的物理内存页数
#define MAP_NR(addr) (((addr) - LOW_MEM) >> 12) // 指定内存地址映射位页号
#define USED 100                                // 页面被占用标志

// 设置内存最高端
static long HIGH_MEMORY = 0;

//zhu yi ci chu
#define copy_page(from,to) \
    __asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024))

static unsigned char mem_map [PAGING_PAGES] = {0,};

unsigned long get_free_page(void){
    register unsigned long __res asm("ax");
    /*__asm__("std ; repne ; scasb\n\t"
        "jne 1fn\t"
        "movb $1,1(%%edi)\n\t"
        "sall $12,%%ecx\n\t"
        "addl %2,%%ecx\n\t"
        "movl %%ecx,%%edx\n\t"
        "movl $1024,%%ecx\n\t"
        "leal 4092(%%edx),%%edi\n\t"
        "rep ; stosl\n\t"
        "movl %%edx,%%eax\n"
        "1:"
        :"=a" (__res)
        :"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),
        "D" (mem_map+PAGING_PAGES-1)
        :"di","cx","dx");*/
    return __res;
}

void free_page(unsigned long addr){
    if(addr < LOW_MEM) return;
    if(addr >= HIGH_MEMORY){
        panic("trying to free nonexistent page");
    }
    addr -= LOW_MEM;
    addr >>= 12;
    if (mem_map[addr]--) return;
    mem_map[addr] = 0;
    panic("tyring to free free page");
}

unsigned long put_page(unsigned long page, unsigned long address){
    unsigned long tmp, *page_table;

    if(page < LOW_MEM || page >= HIGH_MEMORY){
        printk("Try to put page %p at %p\n", page, address);
    }
    if(mem_map[(page-LOW_MEM)>>12] != 1){
        printk("mem_map disagrees with %p at p\n",page, address);
    }
    page_table = (unsigned long *)((address>>20) & 0xffc);
    if((*page_table)&1){
        page_table = (unsigned long *)(0xfffff000 & *page_table);
    }else{
        if(!(tmp=get_free_page())){
            return 0;
        }
        *page_table = tmp | 7;
        page_table = (unsigned long *) tmp;
    }
    page_table[(address>>12) & 0x3ff] = page | 7;
    return page;
}

void un_wp_page(unsigned long * table_entry){
    unsigned long old_page, new_page;

    old_page = 0xfffff000 & *table_entry;
    if(old_page >=  LOW_MEM && mem_map[MAP_NR(old_page)]==1){
        *table_entry |= 2;
        invalidate();
        return;
    }
    if(!(new_page = get_free_page())){
        oom();
    }
    if(old_page >= LOW_MEM){
        mem_map[MAP_NR(old_page)]--;
    }
    *table_entry = new_page |7;
    invalidate();
    copy_page(old_page, new_page);
}

static int share_page(unsigned long address){
    struct task_struct ** p;

    if(!current->executable){
        return 0;
    }
    if(current->executable->i_count < 2){
        return 0;
    }
    for (p = &LAST_TASK; p > &FIRST_TASK; --p){
        if (!*p){
            continue;
        }
        if (current == *p){
            continue;
        }
        if ((*p)->executable != current->executable){
            continue;
        }
        if (try_to_share(address, *p)){
            return 1;
        }
    }
    return 0;
}

void do_no_page(unsigned long error_code, unsigned long address){
    int nr[4];
    unsigned long tmp;
    unsigned long page;
    int block, i;

    address &= 0xfffff000;
    tmp = address - current->start_code;
    if(!current->executable || tmp >= current->end_data){
        get_empty_page(address);
        return;
    }
    if(share_page(tmp)){
        return;
    }
    if(!(page = get_free_page())){
        oom();
    }
    block = 1 + tmp/BLOCK_SIZE;
    for (i=0; i < 4; block++,i++){
        nr[i] = bmap(current->executable,block);
        bread_page(page, current->executable->i_dev,nr);
        i = tmp +4096;
        while(i-- > 0){
            tmp--;
            *(char *)tmp = 0;
        }
        if(put_page(page, address)){
            return;
        }
        free_page(page);
        oom();
    }
}

void do_wp_page(unsigned long error_code, unsigned long address){
#if 0
    if(CODE_SPACE(address)){
        do_exit(SIGSEGV);
    }
#endif
    un_wp_page((unsigned long *)(((address >> 10) & 0xffc) +
                                 (0xfffff000 & *((unsigned long *)
                                                 *(unsigned long *)
                                                 ((address>>20) & 0xffc)))));
}

void write_verify(unsigned long address){
    unsigned long page;
    if (!((page = *((unsigned long *)((address>>20) & 0xffc)))&1)){
        return;
    }
    page &= 0xfffff000;
    page += ((address>>10) & 0xffc);
    if((3 & *(unsigned long *) page) ==1){
        un_wp_page((unsigned long *) page);
    }
    return;
}

void get_empty_page(unsigned long address){
    unsigned long tmp;

    if(!(tmp=get_free_page()) || !put_page(tmp, address)){
        free_page(tmp);
        oom();
    }
}

static int try_to_share(unsigned long address, struct task_struct * p){
    /*
    unsigned long from;
    unsigned long to;
    unsigned long from_page;
    unsigned long to_page;
    unsigned long phys_addr;
    */
    return 1;
}

void mem_init(long start_mem, long end_mem){
    int i;

    HIGH_MEMORY = end_mem;
    for (i = 0; i < PAGING_PAGES; i++){
        mem_map[i] = USED;          //将页面映射数组全置成USED
    }
    i = MAP_NR(start_mem);          //计算可以使用起始内存的页面号
    end_mem -= start_mem;           //再计算可用于分页处理的内存块大小
    end_mem >>= 12;                 // 计算可用于分页处理的页面数
    while (end_mem-->0){
        mem_map[i++] = 0;           // 将这些可用页面对应的页面映射数组清0
    }
}
