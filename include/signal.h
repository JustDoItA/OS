#ifndef _SIGNAL_H
#define _SIGNAL_H

#define SIGKILL 9
#define SIGALRM 14
#define SIGSTOP 19

typedef unsigned int sigset_t;


struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

#endif
