/*
 *@lpf
 *Sun Nov 19 01:16:15 PST 2017
 */
#define __LIBRARY__
#include <unistd.h>
#include <sys/wait.h>


_syscall3(pid_t, waitpid, pid_t, pid, int *, wait_stat, int, options)

pid_t wait(int * wait_stat){
    return waitpid(-1, wait_stat, 0);
}
