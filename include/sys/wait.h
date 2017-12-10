/*
 *@lpf 
 *Sun Nov 19 01:24:27 PST 2017
 * */
#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

#endif
