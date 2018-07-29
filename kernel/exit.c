/*
 *@lpf
 *Sun Oct 15 00:23:10 PDT 2017
 */

int do_exit(long code){
    //int i;
    return -1;
}

int sys_exit(int error_code){
    return do_exit((error_code&0xff)<<8);
}

int sys_waitpid(){
    return 0;
}
