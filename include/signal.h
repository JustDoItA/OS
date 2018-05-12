#ifndef _SIGNAL_H
#define _SIGNAL_H


#define SIGINT  2
#define SIGQUIT 3

#define SIGFPE 8
#define SIGKILL 9
#define SIGSEGV 11
#define SIGALRM 14
#define SIGCHLD 17
#define SIGSTOP 19

#define SA_NOMASK  0x4000000
#define SA_ONESHOT 0x8000000

typedef unsigned int sigset_t;


struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

#endif
