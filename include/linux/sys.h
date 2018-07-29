/*
    @lpf
    Fri Sep 15 20:23:42 PDT 2017
 */
extern int sys_setup();
extern int sys_exit();
extern int sys_fork();
extern int sys_read();
extern int sys_write();
extern int sys_open();
extern int sys_close();
extern int sys_waitpid();
extern int sys_creat();
extern int sys_link();
extern int sys_unlink();
extern int sys_execve();
extern int sys_chdir();
extern int sys_time();
extern int sys_mknod();
extern int sys_chmod();
extern int sys_chown();
extern int sys_break();
extern int sys_stat();
extern int sys_lseek();
extern int sys_getpid();
extern int sys_mount();
extern int sys_umount();
extern int sys_setuid();
extern int sys_getuid();
extern int sys_stime();
extern int sys_ptrace();
extern int sys_alarm();
extern int sys_fstat();
extern int sys_pause();


extern int sys_sync();

fn_ptr sys_call_table[] = {
    sys_setup,sys_exit,sys_fork,sys_read,sys_write,sys_open,sys_close,
    sys_waitpid,sys_creat,sys_link,sys_unlink,sys_execve,sys_chdir,
    sys_time,sys_mknod,sys_chmod,sys_chown,sys_break,sys_stat,sys_lseek,
    sys_getpid,sys_mount,sys_umount,sys_setuid,sys_getuid,sys_stime,
    sys_ptrace,sys_alarm,sys_fstat,sys_pause,sys_sync
};
