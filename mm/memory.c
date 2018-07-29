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

//刷新页变换高速缓冲宏函数
//为了提高地址转换效率，CPU将最近使用的页表数据存放在芯片中高速缓冲中
//在修改过页表信息后，就需要刷新该缓冲区，这里使用重新加载页面目录基地址
//寄存器cr3的方法来进行刷新，下面的eax=0，是页面目录的基址
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

//https://blog.csdn.net/cinmyheart/article/details/24967455
//取空闲页面,如果已经没有内存了,则返回0
//输入 %1(ax=0) %2(LOW_MEM) %3(cx=PAGING PAGES) %4(edi=mem_map+PAGING_PAGES-1)
//输出: 返回%0(ax=页面起始地址)
//上面%4寄存器实际执行mem_map[]内存字节图的最后一个字节本函数从字节末端开始向前
//扫描 所有页面标志(页面总数为PAGING_PAGES),若有页面空闲(其内存影像字节为0),则
//返回页面地址，注意！本函数只是指出主内存区的一样空闲页面，并没有映射到某个进程
//的线性地址去，后面的put_page()函数就是用来做映射的

unsigned long get_free_page(void){

    //__res是寄存器级变量,值保存在ax寄存器中,就是说对__res操作等于ax寄存器操作,
    //为效率考虑
    register unsigned long __res asm("ax");

    //std置方向标志位1,DF标志位=1(std设置DF=1，所以scasb执行递减操作)
    //repne/repnez 当不相等/不为0时重复串操作,退出条件cx=0/zf=1
    //scasb 对指定区域扫描制定字符,区域由ES：DI(0x0010:0x00014bff[mem_map+PAGING_PAGES-1])
    //及ECX(0x00000f00)指定，字符由AL指定 如果ES:DI指向该字符cx=0

    //"std ; repne ; scasb\n\t" repne scasb al, byte ptr es:[edi] ; 执行完之后ecx-1 edx-1
    //"movb $1,1(%%edi)\n\t"-> mov byte ptr ds:[edi+1]
    //sall $12,%%ecx ecx内容左移12位
    //"addl %2,%%ecx\n\t" LOW_MEM+0xeff(汇编第一行比较时0ff-1)<<12
    // "movl %%ecx,%%edx\n\t" 将ecx(0x00fff000)即页面地址保存到edx中
    // "movl $1024,%%ecx\n\t" 将1024赋值给ecx 作用是计数器是为了给rep;
    // stosl计数
    //
    //
    // "leal 4092(%%edx),%%edi\n\t"
    // edx+4092得到该页最后一个字节的地址，然后赋值给edi
    // 该语句执行之前 edi = 0x00014bfe
    // 该语句执行之后 edi = 0x00fffffc
    //
    // "rep ; stosl\n\t" edi
    // 所指的内存反向清0[stosl指令相当与将eax中的值保存到es:edi指向的地址中
    // ，若设置了EFLAGS中的方向位（即在STOSL指令前有STD）则EDI自减4，否则使用cld
    // EDI自增4]
    __asm__("std ; repne ; scasb\n\t"
        "jne 1f\n\t"
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
        ); 
        //:"di","cx","dx");
    //al = 0 ;如果mem_map[i]==0,表示为空闲页，否则为已分配占用
    //ecx = PAGEING_PAGES 主内存页表个数
    //es:di=(mem_map+PAGEING_PAGES-1)//内存管理最后一项
    //从数组mem_map[0..(PAGEING_PAGES-1)]
    //的最后一项每比较一次，es:di值减1，如果不相等，ed:di值减1,即mem_map[i--],
    //继续比较，直到ecx==0，如果相等，则跳出循环
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

int free_page_tables(unsigned long from, unsigned long size){
    unsigned long *pg_table;
    unsigned long *dir, nr;

    if(from & 0x3fffff){
        panic("free_page_tables called with wrong alignment");
    }
    if(!from){
        panic("Trying to free up swapper memory space");
    }
    size = (size + 0x3fffff) >> 22;
    dir = (unsigned long *) ((from>>20) & 0xffc);
    for( ; size-->0; dir++){
        if(!(1 & *dir)){
            continue;
        }
        pg_table = (unsigned long *) (0xfffff000 & *dir);
        for(nr = 0; nr < 1024; nr++){
            if(1 & *pg_table){
                free_page(0xfffff000 & *pg_table);
            }
            *pg_table = 0;
            pg_table++;
        }
        free_page(0xfffff000 & *dir);
        *dir = 0;
    }
    invalidate();
    return 0;
}

//下面是内存管理mm中最复杂的程序之一，它通过只复制内存页面来拷贝一定范围内线性
//地址中的内容
//
//注意我们并不是仅复杂任何内存块
//内存块的地址需要4MB的倍数（正好一个页目录项对应的内存大小）,因为这样处理可以使
//函数很简单,不管怎样他仅被fork使用
//
//
//注意2 当from==0时，是在为第一次for调用复制内裤空间。此时我们不想复制整个
//目录对应的内存，因为这样做会导致内存严重的浪费，我们只复制头160个页面
//对应640kb，即使是复制这些页面也已经超出我们的需求
//但这不会占用更多的内存 在低1MB内存范围内我们不执行写时复制操作，所以这些页面
//可以与内核共享，因此这是nr=xxxx的特殊情况（nr在程序中指的是页面数）
//
//复制指定线性地址和长度(页表个数),内存对应的页目录和页表，从而被复制的页目录和
//页表对应的原物理内存区被共享使用
//
//复制指定地址和长度的内存对应的页目录和页表项，需要申请页面来存放新页表，原
//内存区被共享，复制指定地址和长度对应的页目录和页表项，需要申请页面来存放新
//页表，原内存区被共享 次后两个进程将共享内存区，直到有一个新进程执行写操作
//才分配新的内存页(写时复制机制)
int copy_page_tables(unsigned long from, unsigned long to, long size){
    unsigned long * from_page_table;
    unsigned long * to_page_table;
    unsigned long this_page;
    unsigned long * from_dir, * to_dir;
    unsigned long  nr;

    //源地址和目的地址都需要4MB的内存边界地址上否则出错死机
    if((from&0x3fffff) || (to&0x3fffff)){
        panic("copy_page_tables called with wwrong alignment");
    }
    //取得源地址和目的地址的目录项(from_dir和to_dir)
    from_dir = (unsigned long *) ((from >> 20) & 0xffc);
    to_dir = (unsigned long *) ((to >> 20) & 0xffc);
    //计算要复制的内存块占用的页表数(也即目录项数)
    size = ((unsigned) (size + 0x3fffff)) >> 22;
    //羡慕开始对每个占用的页表异常进行复制操作
    for( ; size-->0; from_dir++, to_dir++){
        //如果目的目录项指定的页表已经存在(P=1),则出错，死机
        if(1 & *to_dir){
            panic("copy_page_tables: already exist");
        }
        //如果此源目录未被使用，则不用复制对应页表，跳过
        if(!(1 & *from_dir)){
            continue;
        }
        //取当前源目录项中页表的地址
        from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
        //为目的的页表取一项空闲内存，如果是0则说明没有申请到空闲内存页面返回-1
        //退出
        if(!(to_page_table = (unsigned long *) get_free_page())){
            return -1;
        }
        //设置目的目录项信息。7是标志信息，表示(Usr,R/W,Present)
        *to_dir = ((unsigned long) to_page_table) | 7;
        //针对当前初一的页表设置需要复制的页面数，如果是在内核空间，则仅需要复制
        //头160页，否则需要复制1个页表中的1024页面
        nr = (from == 0)?0xA0:1024;
        //对于当前页表，开始复制指定数目nr个页面内存页面
        for( ; nr-- > 0; from_page_table++, to_page_table++){
            this_page = *from_page_table; //取源页表项内容
            if(!(1 & this_page)){ //如果当前页面没有使用，则不用复制
                continue;
            }
            //复位页表中R/W标志(置0),如果是U/S位是0，则R/W就没有作用。如果U/S是
            //1，则R/W是0，那么运行在用户层的代码就只能读页面，如果U/S和R/W都置
            //位，则就有写的权限
            this_page &= ~2;
            *to_page_table = this_page; //将该页面表项复制到目的的页表中
            //如果该页表项所执行的页面地址在1M以下，则需要设置内存页面映射数组
            //mem_map[],于是计算页面号，并以它位索引在页面映射数组对应项中增加
            //引用次数
            if(this_page > LOW_MEM){
                //下面这句的含义是领源页表所指内存页也为只读，因为现在开始有两个
                //进程共用内存区了，若其中一个内存需要进行写操作，则hey通过页面异常的写
                //保护处理，为执行写操作的进程分配一页新的空闲页面，也即进行
                //写时复制的操作
                *from_page_table = this_page; //令源页表项也只读
                this_page -= LOW_MEM;
                this_page >>= 12;
                mem_map[this_page]++;
            }
        }
    }
    invalidate();//刷新页变换高速缓冲
    return 0;
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
