#include <stdarg.h>

//myself
#include <linux/fs.h>

#include <linux/sched.h>
#include <asm/system.h>
extern int end;
struct buffer_head * start_buffer = (struct buffer_head *) & end;

struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;


static inline void wait_on_buffer(struct buffer_head *bh){
   /* cli();
    while (bh->b_block){
        sleep_on(&bh->b_wait);
    }
    sti();
    */
}

int sys_sync(){
/*    int i;
    struct buffer_head *bh;

    sysnc_inodes();
    bh = start_bufffer;
    for (i=0; i<NR_BUFFERS; i++, bh++){
        wait_on_buffer(bh);
        if(bh->b_dirt){
            ll_rw_block(WRITE, bh);
        }
    }*/
    return 0;

}

int sync_dev(int dev){
    return 0;
}

#define _hashfn(dev, block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev, block) hash_table[_hashfn(dev, block)]

static inline void remove_from_queues(struct buffer_head *bh){

}

static inline void insert_into_queues(struct buffer_head *bh){

}

static struct buffer_head * find_buffer(int dev, int block){
    struct buffer_head * tmp;

    for(tmp = hash(dev, block); tmp != NULL; tmp = tmp -> b_next){
        if(tmp -> b_dev == dev && tmp -> b_blocknr == block){
            return tmp;
        }
    }
    return NULL;
}

struct buffer_head * get_hash_table (int dev, int block){
    struct buffer_head * bh;

    for(;;) {
        if(!(bh = find_buffer(dev, block))){
            return NULL;
        }
        bh -> b_count++;
        wait_on_buffer(bh);
        if(bh -> b_dev == dev && bh -> b_blocknr == block){
            return bh;
        }
        bh -> b_count--;
    }
}

#define BADNESS(bh) (((bh) -> b_dirt << 1) + (bh) -> b_lock)

struct buffer_head * getblk(int dev, int block){
    struct buffer_head * tmp, * bh;

repeat:
    if((bh = get_hash_table(dev, block))){
        return bh;
    }
    tmp = free_list;
    do{
        if(tmp -> b_count){
            continue;
        }
        if(!bh || BADNESS(tmp)<BADNESS(bh)) {
            bh = tmp;
            if (!BADNESS(tmp)){
                break;
            }
        }
    }while ((tmp = tmp -> b_next_free) != free_list);
    if(!bh){
        sleep_on(&buffer_wait);
        goto repeat;
    }
    wait_on_buffer(bh);
    if(bh -> b_count){
        goto repeat;
    }
    while(bh -> b_dirt){
        sync_dev(bh->b_dev);
        wait_on_buffer(bh);
        if(bh -> b_count){
            goto repeat;
        }
    }

    if(find_buffer(dev, block)){
        goto repeat;
    }
    bh -> b_count = 1;
    bh -> b_dirt = 0;
    bh -> b_uptodate = 0;
    remove_from_queues(bh);
    bh -> b_dev = dev;
    bh -> b_blocknr = block;
    insert_into_queues(bh);
    return bh;

}

void brelse (struct buffer_head *buf){
    if(!buf){
        return;
    }
    wait_on_buffer(buf);
    if(!(buf->b_count--)){
        panic("Trying to free free buffer");
    }
    wake_up(&buffer_wait);
}

struct buffer_head * bread(int dev, int block){
    struct buffer_head * bh;

    if(!(bh = getblk(dev, block))){
        panic("bread: getblk returned NULL\n");
    }
    if(bh -> b_uptodate){
        return bh;
    }
    ll_rw_block(READ, bh);
    wait_on_buffer(bh);
    if(bh->b_uptodate){
        return bh;
    }
    brelse(bh);
    return NULL;
}

#define COPYBLK(from, to) \
    __asm__("cld\n\t" \
        "rep\n\t" \
        "movsl\n\t" \
        ::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
        )
        //:"cx","di","si")

void bread_page(unsigned long address, int dev, int b[4]){
    struct buffer_head * bh[4];
    int i;

    for(i=0; i<4; i++){
        if(b[i]){
            if((bh[i] = getblk(dev, b[i]))){
                if(bh[i]){
                    ll_rw_block(READ, bh[i]);
                }
            }
        }else{
            bh[i] = NULL;
        }
    }
    for(i=0; i<4; i++, address =+ BLOCK_SIZE){
        if(bh[i]){
            wait_on_buffer(bh[i]);
            if(bh[i] -> b_uptodate){
                COPYBLK((unsigned long) bh[i] -> b_data, address);
            }
        }
        brelse(bh[i]);
    }
}

struct buffer_head * breada(int dev, int first, ...){
    va_list args;
    struct buffer_head * bh, *tmp;

    va_start(args, first);
    if(!(bh = getblk(dev, first))){
        panic("bread: getblk returned NULL \n");
    }
    if(!bh -> b_uptodate){
        ll_rw_block(READ,bh);
    }
    while ((first=va_arg(args,int))>=0) {
         tmp = getblk(dev,first);
        if (tmp){
            if(!tmp->b_uptodate){
                ll_rw_block(READA,bh);
             }
            tmp->b_count--;
        }
    }
    va_end(args);
    wait_on_buffer(bh);
    if(bh -> b_uptodate){
        return bh;
    }
    brelse(bh);
    return (NULL);
}
