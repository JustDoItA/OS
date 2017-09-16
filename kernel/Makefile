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
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = sched.o printk.o mktime.o panic.o #sys.o

kernel.o : $(OBJS)
	$(LD) -r -o kernel.o $(OBJS)
	sync
clean:
	rm -f core *.o *.a

mktimes mktime.o : mktime.c ../include/time.h
panic.s panic.o : panic.c ../include/linux/kernel.h
printk.s printk.o : printk.c ../include/linux/kernel.h ../include/linux/stdarg.h
sched.s sched.o : sched.c ../include/linux/sched.h ../include/linux/fs.h \
	../include/linux/mm.h ../include/linux/kernel.h ../include/linux/head.h
#sys.s sys.o : sys.c ../include/linux/sys.h
