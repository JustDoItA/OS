/*
 *@lpf
 *Sat Aug 12 02:45:05 PDT 2017
 */
#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

#define READ 0
#define WRITE 1


#define MAJOR(a) (((unsigned)(a))>>8)
#define MINOR(a) ((a)&0xff)

#define NR_OPEN 20

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int ROOT_DEV;

struct buffer_head {
    char * b_data;
    unsigned long b_blocknr;
    unsigned short b_dev;
    unsigned char b_uptodate;
    unsigned char b_dirt;
    unsigned char b_count;
    unsigned char b_lock;
    struct task_struct * b_wait;
    struct buffer_head * b_prev;
    struct buffer_head * b_next;
    struct buffer_head * b_prev_free;
    struct buffer_head * b_next_free;
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
#endif
