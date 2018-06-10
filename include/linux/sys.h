/*
    @lpf
    Fri Sep 15 20:23:42 PDT 2017
 */
extern int sys_setup();
extern int sys_exit();
extern int sys_fork();

extern int sys_sync();

fn_ptr sys_call_table[] = {
    sys_setup,sys_exit,sys_fork,sys_sync
};
