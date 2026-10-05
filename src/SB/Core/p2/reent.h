#ifndef PS2_REENT_H
#define PS2_REENT_H

// Newlib runtime types recovered from PS2 retail DWARF (xAnim.cpp).
// All member offsets and aggregate sizes are verified against the originals.
struct _reent;
struct _Bigint;

struct tm
{
    signed int tm_sec;
    signed int tm_min;
    signed int tm_hour;
    signed int tm_mday;
    signed int tm_mon;
    signed int tm_year;
    signed int tm_wday;
    signed int tm_yday;
    signed int tm_isdst;
};

struct __sbuf
{
    unsigned char * _base;
    signed int _size;
};

struct __sFILE
{
    unsigned char * _p;
    signed int _r;
    signed int _w;
    signed short _flags;
    signed short _file;
    struct __sbuf _bf;
    signed int _lbfsize;
    void * _cookie;
    signed int (* _read)(void *, char *, signed int);
    signed int (* _write)(void *, char *, signed int);
    signed long (* _seek)(void *, signed long, signed int);
    signed int (* _close)(void *);
    struct __sbuf _ub;
    unsigned char * _up;
    signed int _ur;
    unsigned char _ubuf[3];
    unsigned char _nbuf[1];
    struct __sbuf _lb;
    signed int _blksize;
    signed int _offset;
    struct _reent * _data;
};

struct _atexit
{
    struct _atexit * _next;
    signed int _ind;
    void (* _fns[32])();
};

struct _glue
{
    struct _glue * _next;
    signed int _niobs;
    struct __sFILE * _iobs;
};

struct _reent
{
    signed int _errno;
    struct __sFILE * _stdin;
    struct __sFILE * _stdout;
    struct __sFILE * _stderr;
    signed int _inc;
    char _emergency[25];
    signed int _current_category;
    char * _current_locale;
    signed int __sdidinit;
    void (* __cleanup)(struct _reent *);
    struct _Bigint * _result;
    signed int _result_k;
    struct _Bigint * _p5s;
    struct _Bigint * * _freelist;
    signed int _cvtlen;
    char * _cvtbuf;
    struct
    {
        union
        {
            struct
            {
                unsigned int _unused_rand;
                char * _strtok_last;
                char _asctime_buf[26];
                struct tm _localtime_buf;
                signed int _gamma_signgam;
                unsigned long long _rand_next;
            } _reent;
            struct
            {
                unsigned char * _nextf[30];
                unsigned int _nmalloc[30];
            } _unused;
        };
    } _new;
    struct _atexit * _atexit;
    struct _atexit _atexit0;
    void (* * _sig_func)(signed int);
    struct _glue __sglue;
    struct __sFILE __sf[3];
};

#ifdef __cplusplus
extern "C" {
#endif
extern struct _reent* _impure_ptr;
#ifdef __cplusplus
}
#endif

#endif
