#ifndef _STRING_H_
#define _STRING_H_
extern inline int strlen(const char *s){
    register int __res __asm__("cx");
    __asm__("cld\n\t"
        "repne\n\t"
        "scasb\n\t"
        "notl %0\n\t"
        "decl %0"
        :"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff));//:"di");
    return __res;
}


extern inline void * memcpy(void * dest, const void *src, int n){
    __asm__("cld\n\t"
            "rep\n\t"
            "movsb"
            ::"c" (n),"S" (src),"D" (dest));
            //:"cx","si","di");
    return dest;
}

extern inline void * memset(void * s, char c, int count){
    /* __asm__("cld\n\t"
        "rep\n\t"
        "stosb"
            ::"a" (c),"D" (s),"c" (count)
            :"cx","di");*/
    return s;
}

#endif
