/*
  @lpf
  Sun Nov  5 01:11:53 PST 2017
*/

#include <linux/tty.h>

struct tty_struct tty_table[] = {
    {
        {ICRNL,
         OPOST|ONLCR,
         0,
         ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
         0,
         INIT_C_CC},
         0,
         0,
         con_write,
        {0,0,0,0,""},
        {0,0,0,0,""},
        {0,0,0,0,""}
    },{
        {0,
         0,
        B2400 | CS8,
        0,
        0,
        INIT_C_CC},
        0,
        0,
        rs_write,
        {0x3f8,0,0,0,""},
        {0x3f8,0,0,0,""},
        {0,0,0,0,""}
    },{
        {0,
         0,
         B2400 | CS8,
         0,
         0,
         INIT_C_CC},
        0,
        0,
        rs_write,
        {0x2f8,0,0,0,""},
        {0x2f8,0,0,0,""},
        {0,0,0,0,""}
    }
};

struct tty_queue *table_list[] ={
    &tty_table[0].read_q, &tty_table[0].write_q,
    &tty_table[1].read_q, &tty_table[1].write_q,
    &tty_table[2].read_q, &tty_table[2].write_q,
};

void tty_init(){
    rs_init();
    con_init();
}

void copy_to_cooked(struct tty_struct *tty){

}

void do_tty_interrupt(int tty){
    copy_to_cooked(tty_table + tty);
}
