/*
 *@lpf
 *Sat Aug 12 02:45:05 PDT 2017
 */
#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

#define READ 0
#define WRITE 1
#define READA 2
#define WRITEA 3


#define MAJOR(a) (((unsigned)(a))>>8)
#define MINOR(a) ((a)&0xff)

#define SUPER_MAGIC 0x137F

#define NR_OPEN 20

#define NR_BUFFERS nr_buffers



#define NR_HASH 307

#define BLOCK_SIZE 1024
#define BLOCK_SIZE_BITS 10

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int ROOT_DEV;

//缓冲区头
struct buffer_head {
    char * b_data;                          //指向数据区的指针
    unsigned long b_blocknr;                //块号
    unsigned short b_dev;                   //数据源的设备号
    unsigned char b_uptodate;               //更新标志，表示数据是否已更新
    unsigned char b_dirt;                   //修改标志： 0 未修改，1 已修改
    unsigned char b_count;                  //使用的用户数
    unsigned char b_lock;                   //缓冲区是否被锁定
    struct task_struct * b_wait;            //指向缓冲区解锁的任务(linux/include/sched.h)
    //以下4个指针用于缓冲区的管理
    struct buffer_head * b_prev;            //hash 队列上前一块
    struct buffer_head * b_next;            //hash 队列上下一块
    struct buffer_head * b_prev_free;       //空闲表的前一块
    struct buffer_head * b_next_free;       //空闲表的下一块
};

struct m_inode{
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned long i_size;
    unsigned long i_mtime;
    unsigned char i_gid;
    unsigned char i_nlinks;
    unsigned short i_zone[9];


    struct task_struct * i_wait;
    unsigned long i_atime;
    unsigned long i_ctime;
    unsigned short i_dev;
    unsigned short i_num;
    unsigned short i_count;
    unsigned char i_lock;
    unsigned char i_dirt;
    unsigned char i_pipe;
    unsigned char i_mount;
    unsigned char i_seek;
    unsigned char i_update;
};

struct file {
    unsigned short f_mode;
    unsigned short f_flags;
    unsigned short f_count;
    struct m_inode * f_inode;
    off_t f_pos;
};

struct super_block {
    unsigned short s_ninodes;
    unsigned short s_nzones;
    unsigned short s_imap_blocks;
    unsigned short s_firstdatazone;
    unsigned short s_log_zone_size;
    unsigned short s_max_size;
    unsigned short s_magic;

    struct buffer_head * s_imap[8];
    struct buffer_head * s_zmap[8];
    unsigned short s_dev;
    struct m_inode * s_isup;
    struct m_inode * s_imount;
    unsigned long s_time;
    struct task_struct * s_wait;
    unsigned char s_lock;
    unsigned char s_rd_only;
    unsigned char s_dirt;
};

struct d_super_block {
    unsigned short s_ninodes;
    unsigned short s_nzones;
    unsigned short s_imap_blocks;
    unsigned short s_zmap_blocks;
    unsigned short s_firstdatazone;
    unsigned short s_log_zone_size;
    unsigned long s_max_size;
    unsigned short s_magic;
};

extern int nr_buffers;

extern int bmap(struct m_inode *inde, int block);
extern void ll_rw_block(int rw, struct buffer_head * bh);
extern void brelse(struct buffer_head *buf);

extern struct buffer_head * bread(int dev, int block);
extern void bread_page(unsigned long addr, int dev, int b[4]);
extern struct buffer_head * breada(int dev, int block, ...);
extern int new_block(int dev);

#endif
