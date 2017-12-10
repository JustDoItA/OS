/*
 *@lpf
 *Fri Nov 17 19:30:36 PST 2017
 */
#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

#define __va_round_size(TYPE) \
    (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#ifndef __sparc__
#define va_start(AP, LASTARG) \
(AP=((char *) &(LASTARG) + __va_round_size (LASTARG)))

#else
#define va_start(AP, LASTARG)       \
    (__builtin_saveregs (), \
        AP + ((char *) &(LASTARG) + __va_round_size(LASTARG))
#endif

void va_end (va_list);
#define va_end(AP)

#define va_arg(AP, TYPE)                             \
    (AP += __va_round_size(TYPE),                    \
        *((TYPE *) (AP - __va_round_size (TYPE))))

#endif
