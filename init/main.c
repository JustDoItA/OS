#include "asm/io.h"
#include <time.h> 
#define __LIBRARY__
#include <unistd.h>



static inline _syscall0(int, fork)
static inline _syscall0(int, pause)
static inline _syscall1(int, setup, void *, BIOS)
static inline _syscall0(int, sync)
//myself
static inline _syscall0(int, setsid)


#include <linux/tty.h>
#include <linux/sched.h>

#include <linux/fs.h>
#include <linux/head.h>
#include <asm/system.h>
#include <fcntl.h>
#include <stdarg.h>

static char printbuf[1024];

extern int vsprintf();
extern void init(void);
extern void blk_dev_init(void);
extern void chr_dev_init(void);
extern void hd_init(void);
extern void mem_init(long start, long end);
extern void floppy_init(void);
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

int set_trap(){
    //__asm__("int3");
    return 0;
}

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
    trap_init();
    blk_dev_init();
    chr_dev_init();
    tty_init();
    time_init();
    sched_init();
    buffer_init(buffer_memory_end);
    hd_init();
    floppy_init();
    sti();
    move_to_user_mode();

//调试
//set_trap();
//for(;;){;}
//init();

    if(!fork()){
    init();
    }
    set_trap();

    for(;;){
        set_trap();
        pause();
    }

    return 0;
}

static char * argv_rc[] = {"/bin/sh", NULL};
static char * envp_rc[] = {"/HOME=/", NULL};

static char * argv[] = {"-/bin/sh", NULL};
static char * envp[] = {"HOME=/usr/root", NULL};

void init(void){
    int pid, i;

    //读取硬盘参数包括分区表信息并建立虚拟盘和安装根文件系统设备
    setup((void *) &drive_info);
    (void) open("/dev/tty0",O_RDWR, 0);
    (void) dup(0);
    (void) dup(0);
    //pintf("%d buffers = %d bytes buffer space\n\r", NR_BUFFERS,
    //NR_BUFFERS, NR_BUFFERS*BLOCK_SIZE);
    //printf("Free mem %d bytes\n\r",memory_end-main_memory_start);
    if(!(pid = fork())){
        close(0);
        if(open("/etc/rc",O_RDONLY,0)){
            _exit(1);
        }
        execve("/bin/sh", argv_rc, envp_rc);
        _exit(2);
    }
    if(pid>0){
        while(pid != wait(&i)){

        }
    }
    while(1){
        if((pid=fork())<0){
            //printf("Fork failed in init\r\n");
            continue;
        }
        if(!pid){
            close(0);
            close(1);
            close(2);
            setsid();
            (void) open("/dev/tty0", O_RDWR, 0);
            (void) dup(0);
            (void) dup(0);
            _exit(execve("/bin/sh", argv, envp));
        }
        while(1){
            if(pid == wait(&i)){
                break;
           }
            //printf("\n\rchild %d died with code %04x\n\r, pid, i");
            sync();
        }
    }
    _exit(0);
}
