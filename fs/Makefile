AR =gar
AS =as
CC =gcc
LD =ld
LDFLAGS =-M -x
CFLAGS = -Wall -O -g -fstrength-reduce -fomit-frame-pointer \
	-finline-functions -nostdinc -I../include
CPP = gcc -E -nostdinc -I../include
.c.s:
	$(CC) $(CFLAGE) -S -o $*.o $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = super.o buffer.o

fs.o: $(OBJS)
	$(LD) -r -o fs.o $(OBJS)
clean:
	rm -f core *.o *.a

buffer.o : buffer.c
super.o : super.c ../include/linux/fs.h
