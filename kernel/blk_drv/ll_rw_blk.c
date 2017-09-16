#include "blk.h"
#include  <linux/fs.h>

struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
    {NULL,NULL},
};
