AR =gar
AS =as
CC =gcc
LD =ld
LDFLAGS =-M -x
#-fno-builtin 解决自己实现memset函数的时候发生冲突警告
CFLAGS = -Wall -O -g -fstrength-reduce -fomit-frame-pointer \
	-finline-functions -fno-builtin -nostdinc -I../include
CPP = gcc -E -nostdinc -I../include
.c.s:
	$(CC) $(CFLAGE) -S -o $*.o $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
OBJS = super.o buffer.o inode.o truncate.o bitmap.o read_write.o open.o namei.o \
	   stat.o

fs.o: $(OBJS)
	$(LD) -r -o fs.o $(OBJS)
clean:
	rm -f core *.o *.a

buffer.o : buffer.c ../include/linux/fs.h
super.o : super.c ../include/linux/fs.h ../include/asm/system.h \
	../include/linux/sched.h
inode.o : inode.c ../include/linux/fs.h ../include/linux/kernel.h \
	../include/linux/sched.h ../include/asm/system.h ../include/linux/fs.h \
	../include/string.h ../include/sys/stat.h
truncate.o : truncate.c ../include/sys/stat.h ../include/linux/sched.h
bitmap.o : bitmap.c ../include/linux/fs.h ../include/string.h
read_write.o : read_write.c
open.o : open.c
namei.o : namei.c
stat.o : stat.c
