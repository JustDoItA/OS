AR = ar -rv
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
OBJS = errno.o setsid.o open.o dup.o close.o _exit.o execve.o wait.o write.o

lib.a : $(OBJS)
	$(AR) lib.a $(OBJS)
	sync
clean:
	rm -f core *.o *.a
errno.s errno.o : errno.c
setsid.s setsid.o : setsid.c ../include/unistd.h ../include/sys/types.h
open.s open.o : open.c ../include/unistd.h ../include/stdarg.h
dup.s dup.o : dup.c ../include/unistd.h
write.s write.o : write.c ../include/unistd.h
close.s close.o : close.c ../include/unistd.h
_exit.s _exit.o : _exit.c ../include/unistd.h
execve.s execve.o : execve.c ../include/unistd.h
wait.s wait.o : wait.c ../include/unistd.h
write.s write.o : write.c ../include/unistd.h
