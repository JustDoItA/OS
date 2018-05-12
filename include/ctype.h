/**
 * @lpf 
 *Sat May 12 00:59:42 PDT 2018
 */
#ifndef _CTYPE_H
#define _CTYPE_H

#define _U 0x01
#define _L 0x02
#define _D 0x04
#define _C 0x08
#define _P 0x10
#define _S 0x20
#define _X 0x40
#define _SP 0x80

extern unsigned char _ctype[];
extern int _ctmp;

#define isupper(c) ((_ctype+1)[c]&(_U))


#define tolower(c) (_ctmp=c,isupper(_ctmp)?_ctmp-('A'-'a'):_ctmp)

#endif
