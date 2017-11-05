AR =ar -rv
AS =as
LD =ld
LDFLAGS =-M -x
CC =gcc
CFLAGS =-Wall -O -g -fstrength-reduce -fomit-frame-pointer \
	-finline-functions -nostdinc -I../../include
CPP =gcc -E -nostdinc -I../../include

.c.s:
	$(CC) $(CFLAGE) -S -o $*.o $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = ll_rw_blk.o ramdisk.o #floppy.o

blk_drv.a: $(OBJS)
	$(AR)  blk_drv.a $(OBJS)
	sync
clean:
	rm -f core *.o *.a

floppy.s floppy.o : floppy.c ../../include/linux/sched.h \
	../../include/linux/head.h ../../include/linux/fs.h \
	../../include/sys/types.h
ll_rw_blk.s ll_rw_blk.o : ll_rw_blk.c ../../include/linux/fs.h blk.h \
	../../include/linux/kernel.h ../../include/linux/sched.h \
	../../include/asm/system.h
