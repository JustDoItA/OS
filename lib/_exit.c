/*
 *@lpf
 *Sun Nov 19 00:50:38 PST 2017
 */

#define __LIBRARY__
#include <unistd.h>

void _exit(int exit_code){
    __asm__("int $0x80"::"a" (__NR_exit),"b" (exit_code));
}
