/*
 *@lpf
 *Sun Nov 19 01:04:20 PST 2017
 */

#define __LIBRARY__
#include <unistd.h>

_syscall3(int, execve, const char *, file, char **, argv, char **, envp);
