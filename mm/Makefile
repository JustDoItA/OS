AR = gar
AS = as
LD = ld
LDFLAGS = -M -x
CC = gcc
CFLAGS = -Wall -O -g -fstrength-reduce -fomit-frame-pointer \
		-finline-functions -nostdinc -I../include
CPP = gcc -E -nostdinc -I../include
.c.s:
	$(CC) $(CFLAGS) -S -o $*.o $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = memory.o page.o
mm.o : $(OBJS)
	$(LD) -r -o mm.o $(OBJS)
clean:
	rm -f core *.o *.a

memory.o : memory.c ../include/signal.h ../include/linux/kernel.h \
	../include/linux/sched.h ../include/linux/fs.h ../include/linux/mm.h
