/*
 *@lpf
 *Fri Nov 17 18:21:13 PST 2017
 */
#ifndef _HDREG_H
#define _HDREG_H

struct partition {
    unsigned char boot_int;
    unsigned char head;
    unsigned char sector;
    unsigned char cyl;
    unsigned char sys_ind;
    unsigned char end_head;
    unsigned char end_sector;
    unsigned char end_cyl;
    unsigned int start_sect;
    unsigned int nr_sects;
};

#endif
