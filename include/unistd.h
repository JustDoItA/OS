/*
 *@lpf
 *Sat Nov 11 18:12:06 PST 2017
 */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/times.h>

extern int errno;

#ifdef __LIBRARY__

#define __NR_setup    0
#define __NR_exit     1
#define __NR_fork     2

#define __NR_write    4
#define __NR_open     5
#define __NR_close    6
#define __NR_waitpid  7
#define __NR_execve   11

#define __NR_pause  29
#define __NR_sync   36
#define __NR_dup    41

#define __NR_setsid 66

#define _syscall0(type,name) \
    type name(void) \
    { \
        long __res; \
        __asm__ volatile ("int $0x80" \
                          : "=a" (__res) \
                          : "0" (__NR_##name)); \
        if(__res >= 0) \
            return (type) __res; \
        errno = - __res; \
        return -1; \
    }

#define _syscall1(type, name, atype, a) \
type name (atype a) \
{ \
long __res; \
__asm__ volatile("int $0x80" \
                 : "=a" (__res) \
                 : "0" (__NR_##name),"b" ((long)(a)));  \
if(__res >= 0){ \
    return (type) __res; \
} \
errno = -__res; \
return -1; \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
    type name(atype a,btype b,ctype c) \
    { \
        long __res; \
        __asm__ volatile("int $0x80" \
                         : "=a" (__res) \
                         : "0" (__NR_##name),"b" ((long)(b)),"d" ((long)(c))); \
        if(__res>=0) \
            return (__res);                     \
        errno=-__res; \
        return -1; \
    }


#endif /* __LIBRARY */

int close(int fildes);
int dup(int fildes);
int execve(const char * filename, char **argv, char **envp);


int open(const char * filename, int flag, ...);
//int fork(void);
int write(int fileds,const char *buf, off_t count);

//volatile void _exit(int status);
void _exit(int status);
//void sync(void);
pid_t wait(int * wait_stat);

#endif
