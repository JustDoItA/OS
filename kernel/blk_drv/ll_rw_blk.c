#include <linux/sched.h>
#include "blk.h"
#include  <linux/fs.h>
#include <linux/kernel.h>
#include <asm/system.h>

struct request request[NR_REQUEST];

struct task_struct * wait_for_request = NULL;

struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
};

static inline void lock_buffer(struct buffer_head * bh){
    cli();
    while(bh -> b_lock){
        sleep_on(&bh -> b_wait);
        bh -> b_lock = 1;
        sti();
    }
}

static inline void unlock_buffer(struct buffer_head * bh){
    if(!bh -> b_lock){
        printk("ll_rw_block.c: buffer not locked\n\r");
    }
    bh -> b_lock = 0;
    wake_up(&bh -> b_wait);
}

static void add_request(struct blk_dev_struct * dev, struct request * req){
    struct request * tmp;

    req -> next = NULL;
    cli();
    if(req -> bh){
        req -> bh -> b_dirt = 0;
    }

    if(!(tmp = dev -> current_request)){
        dev -> current_request = req;
        sti();
        (dev -> request_fn)();
        return;
    }

    for( ; tmp->next; tmp = tmp->next){
        if((IN_ORDER(tmp, req) ||
            (!IN_ORDER(tmp, tmp->next) && IN_ORDER(req, tmp->next))))
            break;
    }
    req -> next = tmp -> next;
    tmp -> next = req;
    sti();
}

static void make_request(int major, int rw, struct buffer_head *bh){

    struct request *req;
    int rw_ahead;

    if((rw_ahead = (rw == READA || rw == WRITEA))){
        if(bh -> b_lock){
            return;
        }
        if(rw == READA){
            rw = READ;
        }else{
            rw = WRITE;
        }
    }
    if(rw != READ && rw != WRITE){
        panic("Bad block dev command, must be R/W/RA/WA");
    }
    lock_buffer(bh);
    if(((rw == WRITE && !bh -> b_dirt) || (rw == READ && bh -> b_uptodate))){
        unlock_buffer(bh);
        return;
    }


repeat:

    if(rw == READ){
        req = request + NR_REQUEST;
    }else{
        req = request + ((NR_REQUEST*2)/3);
    }
    while(--req >= request){
        if(req -> dev < 0){
            break;
        }
    }

    if(req < request){
        if(rw_ahead){
            unlock_buffer(bh);
            return;
        }
        sleep_on(&wait_for_request);
        goto repeat;
    }

    req -> dev = bh -> b_dev;
    req -> cmd = rw;
    req -> errors = 0;
    req -> sector = bh -> b_blocknr << 1;
    req -> nr_sectors = 2;
    req -> buffer = bh -> b_data;
    req -> waiting = NULL;
    req -> bh = bh;
    req -> next = NULL;
    add_request(major+blk_dev, req);

}

void ll_rw_block(int rw, struct buffer_head *bh){
    unsigned int major;

    if((major = MAJOR(bh->b_dev)) >= NR_BLK_DEV ||
       !(blk_dev[major].request_fn)){
        printk("Trying to read nonexistent block-device\n\r");
        return;
    }
    make_request(major, rw, bh);
}
//块设备初始化函数，由初始化程序main.c调用
//初始话请求数组，将所有请求项置为空闲项(dev = -1)
//有32项（NR_REQUEST = 32）
void blk_dev_init(void){
    int i;
    for (i=0 ; i<NR_REQUEST ; i++){
        request[i].dev = -1;
        request[i].next = NULL;
    }
}
