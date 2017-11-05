AR = gar
AS = as
LD = ld
#LDFLAGS = -s -x
LDFLAGS =-M -x
CC = gcc
CFLAGS = -Wall -O -g -fstrength-reduce -fomit-frame-pointer \
		 -finline-functions -nostdinc -I../include
CPP = gcc -E -nostdinc -I../include
.c.s:
	$(CC) $(CFLAGS) -S -o $*.o $<
.s.o:
	#$(AS) -c -o $*.o $<
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = sched.o system_call.o printk.o mktime.o panic.o signal.o fork.o exit.o \
	   traps.o asm.o#sys.o

kernel.o : $(OBJS)
	$(LD) -r -o kernel.o $(OBJS)
	sync
clean:
	rm -f core *.o *.a
exit.s exit.o : exit.c
fork.s fork.o : fork.c ../include/linux/sched.h ../include/linux/kernel.h
mktimes mktime.o : mktime.c ../include/time.h
panic.s panic.o : panic.c ../include/linux/kernel.h
printk.s printk.o : printk.c ../include/linux/kernel.h ../include/linux/stdarg.h
sched.s sched.o : sched.c ../include/linux/sched.h ../include/linux/fs.h \
	../include/linux/mm.h ../include/linux/kernel.h ../include/linux/head.h
signal.s signal.o : signal.c ../include/signal.h ../include/linux/sched.h \
	../include/asm/segment.h ../include/linux/kernel.h
traps.s traps.o : traps.c ../include/linux/head.h ../include/linux/sched.h \
	../include/linux/kernel.h ../include/asm/system.h ../include/asm/io.h
#sys.s sys.o : sys.c ../include/linux/sys.h
