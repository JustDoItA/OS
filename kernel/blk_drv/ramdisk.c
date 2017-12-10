#include <string.h>

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#define MAJOR_NR 1
#include "blk.h"

char *rd_start;
int rd_length = 0;
//执行虚拟盘的读写操作
void do_rd_request(void)
{
    int len;
    char *addr;

    INIT_REQUEST;
    addr = rd_start + (CURRENT->sector << 9);
    len = CURRENT->nr_sectors << 9;
    if((MINOR(CURRENT->dev != 1)) || (addr+len > rd_start+rd_length)){
        end_request(0);
        goto repeat;
    }
    if (CURRENT-> cmd == WRITE){
        (void) memcpy(addr, CURRENT->buffer, len);
    }else if(CURRENT->cmd == READ){
        (void) memcpy(CURRENT->buffer ,addr, len);
    } else {
        panic("unknown ramdisk-command");
    }
    end_request(1);
    goto repeat;
}

// 返回虚拟盘ramdisk 所需的内存量
// 虚拟盘初始化函数， 确定虚拟盘在内存中的起始地址长度。并对整个虚拟盘清0
long rd_init(long mem_start, int length)
{
    int i;
    char *cp;

    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
    rd_start = (char *) mem_start;
    rd_length = length;
    cp = rd_start;
    for (i=0; i < length; i++) {
        *cp++ = '\0';
    }
    return (length);
}


void rd_load(void){
    struct buffer_head *bh;
    struct super_block s;
    int block = 256;
    int i = 1;
    int nblocks;
    char *cp;

    if(!rd_length){
        return ;
    }
    printk("Ram disk: %d bytes, starting at 0x%x\n", rd_length,
           (int)rd_start);
    if(MAJOR(ROOT_DEV) != 2){
        return;
    }
    bh = breada(ROOT_DEV,block + 1, block, block + 2, -1);
    if(!bh){
        printk("Disk error while looking for ramdisk!\n");
        return;
    }
    *((struct d_super_block *) &s) = *((struct d_super_block *) bh -> b_data);
        brelse(bh);
    if(s.s_magic != SUPER_MAGIC){
        return;
    }
    nblocks = s.s_nzones << s.s_log_zone_size;
    if (nblocks > (rd_length >> BLOCK_SIZE_BITS)){
        printk("Ram disk image too big! ()",
               nblocks, rd_length >> BLOCK_SIZE_BITS);
        return;
    }
    printk("Loading %d bytes into ram disk ... 0000k",
           nblocks << BLOCK_SIZE_BITS);
    cp = rd_start;
    while (nblocks) {
        if (nblocks > 2){
            bh = breada(ROOT_DEV, block, block + 1, block + 2, -1);
        }else{
            bh = bread(ROOT_DEV, block);
        }
        if (!bh){
            printk("I/O error on block %d, aborting load\n",
                block);
            return;
        }
        (void) memcpy(cp, bh -> b_data, BLOCK_SIZE);
        brelse(bh);
        printk("\010\010\010\010\010%4dk", i);
        cp += BLOCK_SIZE;
        block++;
        nblocks--;
        i++;
    }
    printk("\010\010\010\010\010done \n");
    ROOT_DEV = 0x0101;
}
