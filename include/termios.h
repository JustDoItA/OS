/*
  @lpf
  Sun Nov  5 01:43:00 PST 2017
 */

#ifndef _TERMIOS_H
#define _TERMIOS_H

#define NCCS 17
struct termios {
    unsigned long c_iflag;
    unsigned long c_oflag;
    unsigned long c_cflag;
    unsigned long c_lflag;
    unsigned char c_line;
    unsigned char c_cc[NCCS];
};

/* c_cc characters */
#define VERASE 2


/*c_iflag bits */

#define ICRNL 0000400


/*c_oflag bits */
#define OPOST 0000001

#define ONLCR 0000004

/* c_cflag bit meaning */

#define B2400 0000013

#define CS8 0000060
/* c_lflag bits */
#define ISIG 0000001
#define ICANON 0000001

#define ECHO 0000010

#define ECHOCTL 0001000

#define ECHOKE 0004000


#endif
