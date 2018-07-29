/*
 * linux/fs/super.c
 *@lpf Sat Aug 12 02:35:06 PDT 2017
 */
#include <linux/sched.h>

#include <linux/fs.h>

#include <asm/system.h>

void wait_for_keypress(void);

#define set_bit(bitnr, addr) ({\
            register int __res __asm__("ax");\
            __asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
            __res;})


struct super_block super_block[NR_SUPER];

int ROOT_DEV = 0;

static void lock_super(struct super_block * sb){
    cli();
    while(sb->s_lock){
        sleep_on(&(sb->s_wait));
    }
    sb->s_lock = 1;
    sti();
}

static void free_super(struct super_block * sb){
    cli();
    sb->s_lock = 0;
    wake_up(&(sb->s_wait));
    sti();
}



static void wait_on_super(struct super_block * sb){
    cli();
    while(sb->s_lock){
        sleep_on(&(sb->s_wait));
    }
    sti();
}

struct super_block * get_super(int dev){
    struct super_block *s;

    if(!dev){
        return NULL;
    }
    s = 0+super_block;
    while(s < NR_SUPER+super_block){
        if(s->s_dev == dev){
            wait_on_super(s);
            if(s->s_dev == dev){
                return s;
            }
            s = 0+super_block;
        }else{
            s++;
        }
    }
    return NULL;
}

void put_super(int dev){
    struct super_block * sb;
    //struct m_inode * inode;
    int i;

    if(dev == ROOT_DEV){
        //printk("root diskette changed: prepare for armageddon \n\r");
        return;
    }
    if(!(sb = get_super(dev))){
        return;
    }
    if(sb->s_imount){
        //printk("Mounted disk changed - tssk,tassk\n\r");
        return;
    }
    lock_super(sb);
    sb->s_dev = 0;
    for(i=0; i<I_MAP_SLOTS; i++){
        brelse(sb->s_imap[i]);
    }
    for(i=0; i<Z_MAP_SLOTS; i++){
        brelse(sb->s_imap[i]);
    }
    free_super(sb);
    return;
}

static struct super_block * read_super(int dev){
    struct super_block *s;
    struct buffer_head *bh;
    int i,block;

    if(!dev){
        return NULL;
    }
    check_disk_change(dev);
    if((s = get_super(dev))){
        return s;
    }
    for(s=0+super_block;; s++){
        if(s >= NR_SUPER+super_block){
            return NULL;
        }
        if(!s->s_dev){
            break;
        }
    }
    s->s_dev = dev;
    s->s_isup = NULL;
    s->s_imount = NULL;
    s->s_time = 0;
    s->s_rd_only = 0;
    s->s_dirt = 0;
    lock_super(s);
    if(!(bh = bread(dev,1))){
        s->s_dev=0;
        free_super(s);
        return NULL;
    }

    *((struct d_super_block *) s) = *((struct d_super_block *) bh->b_data);
    brelse(bh);
    if(s->s_magic != SUPER_MAGIC){
        s->s_dev=0;
        free_super(s);
        return NULL;
    }
    for(i=0; i<I_MAP_SLOTS; i++){
        s->s_imap[i] = NULL;
    }
    for(i=0; i<I_MAP_SLOTS; i++){
        s->s_zmap[i] = NULL;
    }
    block = 2;
    for (i=0; i<s->s_imap_blocks; i++){
        if((s->s_imap[i]=bread(dev,block))){
            block++;
        }else{
            break;
        }
    }
    for (i=0; i<s->s_zmap_blocks; i++){
        if((s->s_zmap[i]=bread(dev,block))){
            block++;
        }else{
            break;
        }
    }

    if(block != 2+s->s_imap_blocks+s->s_zmap_blocks){
        for(i=0; i<I_MAP_SLOTS; i++){
            brelse(s->s_imap[i]);
        }
        for(i=0; i<Z_MAP_SLOTS; i++){
            brelse(s->s_zmap[i]);
        }
        s->s_dev = 0;
        free_super(s);
        return NULL;
    }
    s->s_imap[0] -> b_data[0] |= 1;
    s->s_zmap[0] -> b_data[0] |= 1;
    free_super(s);
    return s;

}

int sys_umount(){
    return 0;
}

int sys_mount(){
    return 0;
}

void mount_root(void){
    int i,free;
    struct super_block * p;
    struct m_inode * mi;

    if (32 != sizeof (struct d_inode)){
        //panic("bad i-node size");
    }
    /*for(i=0; i<NR_FILE; i++){
        file_table[i].f_count=0;
        }*/
    if(MAJOR(ROOT_DEV) ==2){
        //printk("Insert root floppy and press ENTER");
        wait_for_keypress();
    }
    for(p = &super_block[0]; p < &super_block[NR_SUPER]; p++){
        p->s_dev = 0;
        p->s_lock = 0;
        p->s_wait = NULL;
    }
    if(!(p=read_super(ROOT_DEV))){
        //panic("Unable to mount root");
    }
    if(!(mi=iget(ROOT_DEV,ROOT_INO))){
        //panic("Unable to read root i-node");
    }
    mi->i_count += 3;
    p->s_isup = p->s_imount = mi;
    current->pwd = mi;
    current->root = mi;
    free = 0;
    i=p->s_nzones;
    while(-- i >= 0){
        if(!set_bit(i&8191,p->s_zmap[i>>13]->b_data)){
            free++;
        }
    }
    //printk("%d/%d free blocks\n\r",free,p->b_data);
    free = 0;
    i=p->s_ninodes+1;
    while(-- i >=0){
        if(!set_bit(i&8191,p->s_imap[i>>13]->b_data)){
            free++;
        }
    }
    //printk("%d/%d free inodes \n\r",free,p->s_ninodes);
}
