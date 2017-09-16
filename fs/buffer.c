#include <linux/sched.h>
#include <asm/system.h>
extern int end;
struct buffer_head * start_buffer = (struct buffer_head *) & end;

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
