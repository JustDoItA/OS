RAMDISK = -DRAMDISK=512
MAKE = make

AS86 = as86 -O -a
LD86 = ld -O
NASM = nasm

AS = gcc
LD = ld
LDFLAGS =-x -M -Ttext 0x0000#-Tinit/main.lds
#LDFLAGS =-s -x -M -Ttext 0x0000#-Tinit/main.lds

CC = gcc $(RAMDISK)

#CFLAGS =-Wall -O -fomit-frame-pointer
#O0 不进行优化 否则在GDB调试时会出现optimized out 错误
CFLAGS =-Wall -O0 -g -fomit-frame-pointer

CPP =cpp -nostdine -Iinclude

ROOT_DEV=/dev/hd6
ARCHIVES=kernel/kernel.o fs/fs.o  mm/mm.o kernel/math/math.a
DRIVERS=kernel/blk_drv/blk_drv.a #kernel/chr_drv.a
MATH = kernel/math/math.a
LIBS = lib/lib.a

.c.s:
	$(CC) $(CFLAGS) -nostdinc -Iinclude -S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) -nostdinc -Iinclude -c -o $*.o $<

all:Image
Image: boot/bootsect boot/setup tools/system
	#cat boot/bootsect > Image
	#cat boot/setup >> Image
disk:
	dd if=boot/bootsect of=boot.img bs=512 count=1 conv=notrunc
	cat boot/setup tools/system >> boot.img
	sync

clean:
	rm -f Image System.map boot/bootsect boot/setup boot.img
	rm -f init/*.o tools/system tools/sys boot/head.o
	(cd kernel; make clean)
	(cd kernel/math; make clean)
	(cd kernel/blk_drv; make clean)
	(cd fs; make clean)
	(cd mm; make clean)

boot/bootsect: boot/boot.asm
	$(NASM) -o boot/bootsect boot/boot.asm
boot/setup: boot/setup.asm
	$(NASM) -o boot/setup boot/setup.asm
boot/head.o: boot/head.s
tools/system: boot/head.o init/main.o
	(cd kernel; make)
	(cd kernel/math; make)
	(cd kernel/blk_drv; make)
	(cd fs; make)
	(cd mm; make)
	$(LD) $(LDFLAGS) boot/head.o init/main.o $(ARCHIVES) $(DRIVERS) -o tools/sys > System.map
	objcopy -O binary -S tools/sys tools/system
init/main.o : init/main.c include/time.h include/linux/sched.h include/linux/head.h
