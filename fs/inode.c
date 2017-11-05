/*
 * @lpf
 * Sat Oct 28 02:17:17 PDT 2017
 */

#include <linux/kernel.h>
#include <linux/sched.h>

//struct m_inode inode_table[NR_INODE] = {{0,},};

static int _bmap (struct m_inode * inode, int block, int create){
    struct buffer_head *bh;
    int i;

    if(block < 0){
        panic("bmap: block<0");
    }
    if(block >= 7+512+512*512){
        panic("bmap: block>big");
    }
    if(block<7){
        if(create && !inode->i_zone[block]){
            if((inode->i_zone[block]=new_block(inode->i_dev))){
                inode->i_ctime=CURRENT_TIME;
                inode->i_dirt=1;
            }
        }
        return inode->i_zone[block];
    }
    block -= 7;
    if(block<512){
        if(create && !inode->i_zone[7]){
            if((inode->i_zone[7]=new_block(inode->i_dev))){
                inode->i_dirt = 1;
                inode->i_ctime=CURRENT_TIME;
            }
        }
        if(!inode->i_zone[7]){
            return 0;
        }
        if(!(bh = bread(inode->i_dev, inode->i_zone[7]))){
            return 0;
        }
        i = ((unsigned short *) (bh->b_data))[block];
        if(create && !i){
            if((i=new_block(inode->i_dev))){
                ((unsigned short *)(bh->b_data))[block] = i;
                bh->b_dirt=1;
            }
        }
        brelse(bh);
        return i;
    }
    block -= 512;
    if(create && !inode->i_zone[8]){
        if((inode->i_zone[8]=new_block(inode->i_dev))){
            inode->i_dirt=1;
            inode->i_ctime=CURRENT_TIME;
        }
    }
    if(!inode->i_zone[8]){
        return 0;
    }
    if(!(bh=bread(inode->i_dev,inode->i_zone[8]))){
        return 0;
    }
    i = ((unsigned short *)bh->b_data)[block>>9];
    if(create && !i){
        if((i=new_block(inode->i_dev))){
            ((unsigned short *)(bh->b_data))[block>>9] = i;
            bh->b_dirt=1;
        }
    }
    brelse(bh);
    if(!i){
        return 0;
    }
    if(!(bh=bread(inode->i_dev,i))){
        return 0;
    }
    i = ((unsigned short *)(bh->b_data))[block&511]=i;
    if(create && !i){
        if((i=new_block(inode->i_dev))){
            ((unsigned short *)(bh->b_data))[block&511]=i;
            bh->b_dirt=1;
        }
    }
    brelse(bh);
    return i;
}

int bmap(struct m_inode * inode, int block){
    return _bmap(inode, block, 0);
}
