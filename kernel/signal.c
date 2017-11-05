/*
 *@lpf
 *Sat Oct 14 21:46:44 PDT 2017
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

#include <signal.h>

void do_exit(int error_code);
void do_signal(long signr, long eax, long ebx, long ecx, long edx,
               long fs, long es, long ds,
               long eip, long cs, long eflags,
               unsigned long *esp, long ss){
    unsigned long sa_handler;
    long old_eip=eip;
    struct sigaction * sa= current->sigaction + signr -1;
    int longs;
    unsigned long *tmp_esp;

    sa_handler = (unsigned long) sa->sa_handler;
    if (sa_handler==1){
        return;
    }
    if(!sa_handler){
        if(signr==SIGCHLD){
            return;
        }else{
            do_exit(1<<(signr-1));
        }
    }
    if(sa->sa_flags & SA_ONESHOT){
        sa->sa_handler = NULL;
        *(&eip) = sa_handler;
        longs = (sa->sa_flags & SA_NOMASK)?7:8;
        *(&esp) -= longs;
        verify_area(esp, longs*4);
        tmp_esp=esp;
        put_fs_long((long) sa->sa_restorer,tmp_esp++);
        put_fs_long(signr, tmp_esp++);
        if(!(sa->sa_flags & SA_NOMASK)){
            put_fs_long(current->blocked,tmp_esp++);
        }
        put_fs_long(eax, tmp_esp++);
        put_fs_long(ecx, tmp_esp++);
        put_fs_long(edx, tmp_esp++);
        put_fs_long(eax, tmp_esp++);
        put_fs_long(eflags, tmp_esp++);
        put_fs_long(old_eip, tmp_esp++);
        current->blocked |= sa->sa_mask;
    }
}
