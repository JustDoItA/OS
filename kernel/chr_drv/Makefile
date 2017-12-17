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

OBJS = tty_io.o console.o serial.o rs_io.o keyboard.o

chr_drv.a: $(OBJS)
	$(AR) chr_drv.a $(OBJS)
	sync
clean:
	rm -f core *.o *.a
keyboard.s : keyboard.S ../../include/linux/config.h
	$(CPP) -traditional keyboard.S -o keyboard.s
tty_io.s tty_io.o : tty_io.c ../../include/linux/tty.h
console.s console.o : console.c ../../include/linux/tty.h \
	../../include/asm/io.h ../../include/asm/system.h \
	../../include/linux/sched.h
serial.s serial.o : serial.c ../../include/linux/tty.h \
	../../include/asm/io.h
