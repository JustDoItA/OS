/*
 * linux/kernel/blk_drv/floppy.c
 * @lpf
 *Sun Aug 13 02:16:22 PDT 2017
 */
#include <linux/fdreg.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/sched.h>

#define MAJOR_NR 2

#include "blk.h"

static int recalibrate = 0;
static int reset = 0;
static int seek = 0;


extern unsigned char current_DOR;

/*#define immoutb_p(val,port)                                           \
    __asm__("outb %0,%1\n\tjmp 1f\n\tjmp 1f\n1"::"a" ((char)(val)),"i" (port));
*/
#define immoutb_p(val, port) \
    __asm__("cld");

#define MAX_ERRORS 8


//myself
extern void do_fd_request(void);
void (* do_floppy) ();

#define MAX_REPLIES 7
static unsigned char reply_buffer[MAX_REPLIES];
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[3])


static struct floppy_struct{
    unsigned int size, sect, head, track, stretch;
    unsigned char gap, rate, spec1;
} floppy_type[] = {
    {0, 0, 0, 0, 0, 0x00, 0x00, 0x00},
    {720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF},
    {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF},
    {720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF},
    {1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF},
    {720, 9, 2, 40, 1, 0x23, 0x01, 0xDF},
    {1440, 9, 2, 80, 1, 0x23, 0x01, 0xDF},
    {2880, 18, 2, 80, 1, 0x1B, 0x00, 0xCF},
};

extern void floppy_interrupt(void);
extern char tmp_floppy_area[1024];

static int cur_spec1 = -1;
static int cur_rate = -1;
static struct floppy_struct * floppy = floppy_type;
static unsigned char current_drive = 0;
static unsigned char sector = 0;

static unsigned char head = 0;
static unsigned char track = 0;
static unsigned char seek_track = 0;

static unsigned char current_track = 255;
static unsigned char command = 0;

unsigned char selected = 0;
struct task_struct * wait_on_floppy_select = NULL;

void floppy_deselect(unsigned int nr){
    if(nr != (current_DOR & 3)){
        //printk("floppy_deselect:  drive not selected\n\r");
    }
    selected = 0;
    wake_up(&wait_on_floppy_select);
}


int floppy_change(unsigned int nr){
repeat:
    floppy_on(nr);
    while((current_DOR & 3) != nr && selected){
        interruptible_sleep_on(&wait_on_floppy_select);
    }
    if((current_DOR & 3) != nr){
        goto repeat;
    }
    if(inb(FD_DIR) & 0x80){
        floppy_off(nr);
        return 1;
    }
    floppy_off(nr);
    return 0;
}

/*#define copy_buffer(from,to)                  \
    __asm__("cld; rep; movsl" \
            ::"c" (BLOCK_SIZE/4),"S"((long)(from)),"D" ((long)(to))   \
            :"cx", "di", "si");
*/
#define copy_buffer(from,to)                      \
    __asm__("cld");

static void setup_DMA(void){
    long addr = (long) CURRENT->buffer;

    cli();
    if(addr >= 0x100000){
        addr = (long) tmp_floppy_area;
        if(command == FD_WRITE){
            copy_buffer(CURRENT->buffer, tmp_floppy_area);
        }
    }
    immoutb_p(4|2,10);

    /*__asm__("outb %%al,$12\n\tjmp 1f\n1: \tjmp 1f\n1:\t"
            "outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1"::
            "a"((char)((command == FD_READ)?DMA_READ:DMA_WRITE)));
    */

    immoutb_p(addr,4);
    addr >>= 8;

    immoutb_p(addr,4);
    addr >>= 8;

    immoutb_p(addr, 0x81);
    immoutb_p(0xff, 5);
    immoutb_p(3, 5);
    immoutb_p(0|2, 10);
    sti();
}


static void output_byte(char byte){
    int counter ;
    unsigned char status;

    if(reset){
        return;
    }
    for(counter = 0; counter < 10000; counter++){
        status=inb_p(FD_STATUS) & (STATUS_READY | STATUS_DIR);
        if(status == STATUS_READY){
            outb(byte,FD_DATA);
            return;
        }
    }
    reset = 1;
    //printk("Unable to send byte to FDC\n\r");
}

static int result(){
    int i =0, counter, status;

    if (reset){
        return -1;
    }
    for (counter=0; counter<10000; counter++){
        status = inb_p(FD_STATUS)&(STATUS_DIR|STATUS_READY|STATUS_BUSY);
        if(status == (STATUS_DIR|STATUS_READY|STATUS_BUSY)){
            if(i>=MAX_REPLIES){
                break;
            }
            reply_buffer[i++] = inb_p(FD_DATA);
        }
    }
    reset = 1;
    //printk("Getstatus times out \n\r");
    return -1;
}

static void bad_flp_intr(void){
    CURRENT->errors++;
    if(CURRENT->errors > MAX_ERRORS){
        floppy_deselect(current_drive);
        end_request(0);
    }
    if(CURRENT->errors > MAX_ERRORS/2){
        reset = 1;
    }else{
        recalibrate = 1;
    }
}

static void rw_interrupt(void){
    if(result() != 7||(ST0 & 0xf8)||(ST1 & 0xbf)||(ST2 & 0x73)){
        if(ST1 & 0x02){
            //printk("Drive %d is wirte protected\n\r,current_drive");
            floppy_deselect(current_drive);
            end_request(0);
        }else{
            bad_flp_intr();
        }
        do_fd_request();
        return;
    }
    if(command == FD_READ && (unsigned long)(CURRENT->buffer) >= 0x100000){
        copy_buffer(tmp_floppy_area,CURRENT->buffer);
    }
    floppy_deselect(current_drive);
    end_request(1);
    do_fd_request();
}

inline void setup_rw_floppy(){
    setup_DMA();
    do_floppy = rw_interrupt;
    output_byte(command);
    output_byte(head<<2 | current_drive);
    output_byte(track);
    output_byte(head);
    output_byte(sector);
    output_byte(2);
    output_byte(floppy->sect);
    output_byte(floppy->gap);
    output_byte(0xFF);
    if(reset){
        do_fd_request();
    }
}

static void seek_interrupt(void){
    output_byte(FD_SENSEI);
    if(result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track){
        bad_flp_intr();
        do_fd_request();
        return;
    }
    current_track = ST1;
    setup_rw_floppy();
}


static void transfer(void){
    if(cur_spec1 != floppy->spec1){
        cur_spec1 = floppy->spec1;
        output_byte(FD_SPECIFY);
        output_byte(cur_spec1);
        output_byte(6);
    }
    if(cur_rate != floppy->rate){
        outb_p(cur_rate = floppy->rate, FD_DCR);
    }
    if(reset){
        do_fd_request();
        return;
    }
    if(!seek){
        setup_rw_floppy();
        return;
    }
    do_floppy = seek_interrupt;
    if(seek_track){
        output_byte(FD_SEEK);
        output_byte(head<<2 | current_drive);
        output_byte(seek_track);
    }else{
        output_byte(FD_RECALIBRATE);
        output_byte(head<<2 | current_drive);
    }
    if(reset){
        do_fd_request();
    }
}


static void recal_interrupt(void){
    output_byte(FD_SENSEI);
    if(result()!=2 || (ST0 & 0xE0) == 0x60){
        reset = 1;
    }else{
        recalibrate = 0;
    }
    do_fd_request();
}

void unexpected_floppy_interrupt(void){
    output_byte(FD_SENSEI);
    if(result()!=2 || (ST0 & 0xE0) == 0x60){
        reset = 1;
    }else {
        recalibrate = 1;
    }
}

static void recalibrate_floppy(void){
    recalibrate = 0;
    current_track = 0;
    do_floppy = recal_interrupt;
    output_byte(FD_RECALIBRATE);
    output_byte(head<<2 | current_drive);
    if(reset){
        do_fd_request();
    }
}

static void reset_interrupt(void){
    output_byte(FD_SENSEI);
    (void) result();
    output_byte(FD_SPECIFY);
    output_byte(cur_spec1);
    output_byte(6);
    do_fd_request();
}

static void reset_floppy(void){
    int i;

    reset = 0;
    cur_spec1 = -1;
    cur_rate = -1;
    recalibrate = 1;
    //printk("Reset-floppy called\n\r");
    cli();
    do_floppy = reset_interrupt;
    outb_p(current_DOR & ~0x04, FD_DOR);
    for(i=0; i<100; i++){
        __asm__("nop");
    }
    outb(current_DOR,FD_DOR);
    sti();
}

static void floppy_on_interrupt(void){
    selected = 1;
    if(current_drive != (current_DOR & 3)){
        current_DOR &= 0xFC;
        current_DOR |= current_drive;
        outb(current_DOR, FD_DOR);
        add_timer(2, &transfer);
    }else{
        transfer();
    }
}

void do_fd_request(void){
    unsigned int block;

    seek = 0;
    if(reset){
        reset_floppy();
        return;
    }
    if(recalibrate){
        recalibrate_floppy();
        return;
    }
    INIT_REQUEST;
    floppy = (MINOR(CURRENT->dev)>>2) + floppy_type;
    if(current_drive != CURRENT_DEV){
        seek = 1;
    }
    current_drive = CURRENT_DEV;
    block = CURRENT->sector;
    if(block+2 > floppy->size){
        end_request(0);
        goto repeat;
    }
    sector = block % floppy->sect;
    block /=floppy->sect;
    head = block % floppy->head;
    track = block / floppy->head;
    seek_track = track << floppy->stretch;
    if(seek_track != current_track){
        seek = 1;
    }
    sector++;
    if(CURRENT->cmd == READ){
        command = FD_READ;
    }else if(CURRENT->cmd == WRITE){
        command = FD_WRITE;
    }else{
        //panic("do_fd_request: unknow command");
    }
    add_timer(ticks_to_floppy_on(current_drive), &floppy_on_interrupt);

}

void floppy_init(void){
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
    set_trap_gate(0x26,&floppy_interrupt);
    outb(inb_p(0x21)&~0x40,0x21);
}
