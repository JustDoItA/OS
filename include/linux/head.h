/*
 * @lpf Fri Aug 18 19:57:15 PDT 2017
 */
#ifndef _HEAD_H
#define _HEAD_H

typedef struct desc_struct {
    unsigned long a,b;
} desc_table[256];

extern unsigned long pg_dir[1024];

#endif
