/*
 *@lpf
 *Sat Nov 18 20:32:38 PST 2017
 */

#define __LIBRARY__
#include <unistd.h>

_syscall3(int, write, int, fd, const char *, buf, off_t, count)
