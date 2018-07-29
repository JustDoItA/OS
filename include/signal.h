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

//下面是sigaction的数据结构
//sa_handler 是在对应某信号指定要采取的行动。可以是上面的SIG_DFL,或
//者是SIG_IGN来忽略该信号，也可以是指向处理该信号函数的一个指针
//sa_mask给处理对信号的屏蔽码，在信号程序执行时将阻塞对这些信号的处理 
//sa_flags 指定改变信号处理过程的信号集
//sa_restorer 恢复过程指针，是用于保存原返回的过程指针
//另外，引起触发信号处理的信号页将被阻塞，除非使用SA_NOMASK标志
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

#endif
