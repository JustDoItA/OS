#include "asm/io.h"
#include <time.h>

#include <linux/fs.h>
#include <linux/head.h>

#define __LIBRARY__

extern void mem_init(long start, long end);
extern long rd_init(long mem_start, int length);
extern long kernel_mktime(struct tm *tm);
extern long startup_time;

//EXT_MEM_K 64515kb
#define EXT_MEM_K (*(unsigned short *) 0x90002)
//0x306
#define ORIG_ROOT_DEV (*(unsigned short *) 0x901FC)
#define DRIVE_INFO (*(struct drive_info *) 0x90080)

#define CMOS_READ(addr)({\
        outb_p(0x80|addr, 0x70);\
        inb_p(0x71);\
        })

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

static void time_init(void){
    struct tm time;

    do{
        time.tm_sec = CMOS_READ(0);
        time.tm_min = CMOS_READ(2);
        time.tm_hour = CMOS_READ(4);
        time.tm_mday = CMOS_READ(7);
        time.tm_mon = CMOS_READ(8);
        time.tm_year = CMOS_READ(9);
    }while(time.tm_sec != CMOS_READ(0));
    BCD_TO_BIN(time.tm_sec);
    BCD_TO_BIN(time.tm_min);
    BCD_TO_BIN(time.tm_hour);
    BCD_TO_BIN(time.tm_mday);
    BCD_TO_BIN(time.tm_mon);
    BCD_TO_BIN(time.tm_year);
    time.tm_mon--;
    startup_time = kernel_mktime(&time);
}

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;
struct drive_info {char dummy[32];} drive_info;

int main(void){
    ROOT_DEV = ORIG_ROOT_DEV;
    drive_info = DRIVE_INFO;
    memory_end = (1<<20) + (EXT_MEM_K<<10);
    memory_end &= 0xfffff000;
    if (memory_end > 16*1024*1024)
        memory_end = 16*1024*1024;
    if (memory_end > 12*1024*1024)
        buffer_memory_end = 4*1024*1024;
    else if (memory_end > 6*1024*1024)
        buffer_memory_end = 2*1024*1024;
    else
        buffer_memory_end = 1*1024*1024;
    main_memory_start = buffer_memory_end;
#ifdef RAMDISK
    //如果定义了虚拟盘，则将组内存其实部分分给虚拟盘
    main_memory_start += rd_init(main_memory_start, RAMDISK*1024);
#endif
    mem_init(main_memory_start,memory_end);
    time_init();
    //struct test T1;
    //T1.b = 8;
    //printf("Hello %d World!\n",T1.b);
    return 0;
}
