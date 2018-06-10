/*
 *@lpf include/sys/stat.h
 *Sat Jun  9 03:02:02 PDT 2018
 */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#define S_IFMT 00170000
#define S_IFREG 0100000

#define S_IFDIR 0040000


#define S_IFBLK 0060000


#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)


#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)


#endif
