/*
 * @lpf Fri Aug 18 19:57:15 PDT 2017
 */
#ifndef _HEAD_H
#define _HEAD_H

//定义了段描述符的数据结构，该结构仅说明每个描述符是有8个
//字节构成，描述符表共有256项
typedef struct desc_struct {
    unsigned long a,b;
} desc_table[256];

extern unsigned long pg_dir[1024];
extern desc_table idt, gdt;

#endif
