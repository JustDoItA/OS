AR =ar -rv
AS =as
LD =ld
LDFLAGS =-M -x
CC =gcc
CFLAGS =-Wall -O -g -fstrength-reduce -fomit-frame-pointer \
	-finline-functions -fno-builtin -nostdinc -I../../include
CPP =gcc -E -nostdinc -I../../include

.c.s:
	$(CC) $(CFLAGE) -S -o $*.o $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

OBJS = ll_rw_blk.o ramdisk.o hd.o floppy.o

blk_drv.a: $(OBJS)
	$(AR)  blk_drv.a $(OBJS)
	sync
clean:
	rm -f core *.o *.a

floppy.s floppy.o : floppy.c ../../include/linux/sched.h \
	../../include/linux/head.h ../../include/linux/fs.h \
	../../include/sys/types.h ./blk.h
ll_rw_blk.s ll_rw_blk.o : ll_rw_blk.c ../../include/linux/fs.h blk.h \
	../../include/linux/kernel.h ../../include/linux/sched.h \
	../../include/asm/system.h
hd.s hd.o : hd.c ../../include/linux/fs.h ../../include/linux/hdreg.h \
	../../include/asm/io.h ../../kernel/blk_drv/blk.h ../../include/linux/sched.h \
	../../include/asm/system.h
floppy.s floppy.o : floppy.c ../../include/linux/fdreg.h ../../include/asm/system.h \
	../../include/asm/io.h
