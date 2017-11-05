AR = ar
AS = as
LD = ld
#LDFLAGS = -s -x
LDFLAGS =-M -x
CC = gcc
CFLAGS = -Wall -O -g -fstrength-reduce -fomit-frame-pointer \
		 -finline-functions -nostdinc -I../../include
CPP = gcc -E -nostdinc -I../../include
.c.s:
	$(CC) $(CFLAGS) -S -o $*.o $<
.s.o:
	#$(AS) -c -o $*.o $<
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = math_emulate.o

math.o : $(OBJS)
	$(AR) rcs math.a $(OBJS)
	sync
clean:
	rm -f core *.o *.a
math_emulate.s math_emulate.o : math_emulate.c ../../include/linux/sched.h \
	../../include/linux/kernel.h ../../include/asm/segment.h
