#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV 7 // 块设备数量


#define NR_REQUEST 32

struct request {
    int dev;                        //使用的设备号
    int cmd;                        //命令读或写
    int errors;                     //错误次数
    unsigned long sector;           //起始扇区(1块=2个扇区)
    unsigned long nr_sectors;       //读写扇区数
    char *buffer;                   //数据缓冲区
    struct task_struct *waiting;    //任务等待操作执行完成的地方
    struct buffer_head *bh;         //缓冲区头指针(include/linux/fs.h,68)
    struct request * next;          //指向下一请求项
};

#define IN_ORDER(s1, s2) \
    (((s1) -> cmd < (s2) -> cmd )      \
     || ((s1) -> cmd == (s2) -> cmd && \
        ((s1) -> dev < (s2) -> dev)) \
     || ((s1) -> dev == (s2) -> dev && (s1) -> sector < (s2) -> sector))

//块儿设备结构
struct blk_dev_struct
{
    void (*request_fn)(void); //请求操作函数
    struct request *current_request; //请求信息结构
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];

extern struct task_struct *wait_for_request;

#ifdef MAJOR_NR
#if (MAJOR_NR == 1)
//RAM盘 即虚拟盘
#define DEVICE_NAME "ramdisk"
#define DEVICE_REQUEST do_rd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#elif (MAJOR_NR ==2)
//软驱 主设备号是2
#define DEVICE_NAME "floppy"
#define DEVICE_INTR do_floppy
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))

#elif (MAJOR_NR ==3)
//
#define DEVICE_NAME "harddisk"
#define DEVICE_INTR do_hd
#define DEVICE_REQUEST do_hd_request
#define DEVICE_NR(device) (MINOR(device)/5)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
//#elif
#else
#error "unknown blk device"
#endif

#define CURRENT (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV (DEVICE_NR(CURRENT->dev))

extern inline void unlock_buffer(struct buffer_head *bh)
{
    if (!bh->b_lock)
        //printk(DEVICE_NAME": free buffer being unloceed\n");
    bh->b_lock=0;
    wake_up(&bh->b_wait);
}

extern inline void end_request(int uptodate)
{
    DEVICE_OFF(CURRENT->dev);
    if (CURRENT->bh){
        CURRENT->bh->b_uptodate = uptodate;
        unlock_buffer(CURRENT->bh);
    }
    if (!uptodate){
        //  printk(DEVICE_NAME "I/O error \n\r");
        //printk("dev %04x, block %d\n\r",CURRENT->dev,
        //     CURRENT->bh->b_blocknr);
    }
    wake_up(&CURRENT->waiting);
    wake_up(&wait_for_request);
    CURRENT->dev = -1;
    CURRENT = CURRENT->next;
}

#define INIT_REQUEST \
repeat: \
    if (!CURRENT) \
        return; \
    if (MAJOR(CURRENT->dev) != MAJOR_NR) \
        panic(DEVICE_NAME ": request list destroyed"); \
    if(CURRENT->bh){ \
        if(!CURRENT->bh->b_lock) \
            panic(DEVICE_NAME ": block not locked"); \
    }

#endif

#endif
