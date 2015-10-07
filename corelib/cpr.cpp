// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015 Cherokees of Idaho.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/cpr.h>

#ifndef  UCOMMON_SYSRUNTIME
#include <stdexcept>
#endif

#include <ucommon/export.h>
#include <ucommon/string.h>
#include <ucommon/socket.h>
#include <errno.h>
#include <stdarg.h>

#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>

#ifdef  _MSWINDOWS_
#ifdef  HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

int gettimeofday(struct timeval *tv_, void *tz_)
{
    assert(tv_ != NULL);

#if defined(_MSC_VER) && _MSC_VER >= 1300
    struct __timeb64 tb;
    _ftime64(&tb);
#else
# ifndef __BORLANDC__
    struct _timeb tb;
    _ftime(&tb);
# else
    struct timeb tb;
    ftime(&tb);
# endif
#endif
    tv_->tv_sec = (long)tb.time;
    tv_->tv_usec = tb.millitm * 1000;
    return 0;
}

int setenv(const char *sym, const char *val, int flag)
{
    char buf[128];

    if(!flag) {
        if(GetEnvironmentVariable(sym, buf, sizeof(buf)) > 0)
            return 0;
        if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
            return -1;
    }
    if(SetEnvironmentVariable(sym, val) == 0)
        return -1;
    return 0;
}

#endif

void cpr_runtime_error(const char *str)
{
    assert(str != NULL);

#ifndef UCOMMON_SYSRUNTIME
    throw std::runtime_error(str);
#endif
    abort();
}

#if !defined(_MSWINDOWS_) && !defined(__QNX__)

extern "C" int stricmp(const char *s1, const char *s2)
{
#ifdef  HAVE_STRICMP
    return stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

extern "C" int strnicmp(const char *s1, const char *s2, size_t size)
{
#ifdef  HAVE_STRICMP
    return strnicmp(s1, s2, size);
#else
    return strncasecmp(s1, s2, size);
#endif
}

#endif

// just become we need to get binary types in a specific binary endian order.

extern "C" uint16_t lsb_getshort(uint8_t *b)
{
    assert(b != NULL);
    return (b[1] * 256) + b[0];
}

extern "C" uint32_t lsb_getlong(uint8_t *b)
{
    assert(b != NULL);
    return (b[3] * 16777216l) + (b[2] * 65536l) + (b[1] * 256) + b[0];
}

extern "C" uint16_t msb_getshort(uint8_t *b)
{
    assert(b != NULL);
    return (b[0] * 256) + b[1];
}

extern "C" uint32_t msb_getlong(uint8_t *b)
{
    assert(b != NULL);
    return (b[0] * 16777216l) + (b[1] * 65536l) + (b[2] * 256) + b[3];
}

extern "C" void lsb_setshort(uint8_t *b, uint16_t v)
{
    assert(b != NULL);
    b[0] = v & 0x0ff;
    b[1] = (v / 256) & 0xff;
}

extern "C" void msb_setshort(uint8_t *b, uint16_t v)
{
    assert(b != NULL);
    b[1] = v & 0x0ff;
    b[0] = (v / 256) & 0xff;
}

// oh, and we have to be able to set them in order too...

extern "C" void lsb_setlong(uint8_t *b, uint32_t v)
{
    assert(b != NULL);
    b[0] = (uint8_t)(v & 0x0ff);
    b[1] = (uint8_t)((v / 256) & 0xff);
    b[2] = (uint8_t)((v / 65536l) & 0xff);
    b[3] = (uint8_t)((v / 16777216l) & 0xff);
}

extern "C" void msb_setlong(uint8_t *b, uint32_t v)
{
    assert(b != NULL);
    b[3] = (uint8_t)(v & 0x0ff);
    b[2] = (uint8_t)((v / 256) & 0xff);
    b[1] = (uint8_t)((v / 65536l) & 0xff);
    b[0] = (uint8_t)((v / 16777216l) & 0xff);
}

extern "C" void *cpr_newp(void **handle, size_t size)
{
    assert(handle != NULL);
    if(*handle)
        free(*handle);
    *handle = malloc(size);
    return *handle;
}

extern "C" void cpr_freep(void **handle)
{
    assert(handle != NULL);
    if(*handle) {
        free(*handle);
        *handle = NULL;
    }
}

extern "C" void cpr_memswap(void *s1, void *s2, size_t size)
{
    assert(s1 != NULL);
    assert(s2 != NULL);
    assert(size > 0);

    char *buf = new char[size];
    memcpy(buf, s1, size);
    memcpy(s1, s2, size);
    memcpy(s2, buf, size);
    delete[] buf;
}

// if malloc ever fails, we probably should consider that a critical error and
// kill the leaky dingy, which this does for us here..

extern "C" void *cpr_memalloc(size_t size)
{
    void *mem;

    if(!size)
        ++size;

    mem = malloc(size);
    assert(mem != NULL);
    return mem;
}

extern "C" void *cpr_memassign(size_t size, caddr_t addr, size_t max)
{
    assert(addr);
    assert(size <= max);
    return addr;
}

#ifdef UCOMMON_SYSRUNTIME

#ifdef  __GNUC__

// here we have one of those magic things in gcc, and what to do when
// we have an unimplemented virtual function if we build ucommon without
// a stdc++ runtime library.

extern "C" void __cxa_pure_virtual(void)
{
    abort();
}

#endif

void *operator new(size_t size)
{
    return cpr_memalloc(size);
}

void *operator new[](size_t size)
{
    return cpr_memalloc(size);
}

void *operator new[](size_t size, void *address)
{
    return cpr_memassign(size, address, size);
}

void *operator new[](size_t size, void *address, size_t known)
{
    return cpr_memassign(size, address, known);
}

#if __cplusplus <= 199711L
void operator delete(void *object)
#else
void operator delete(void *object) noexcept (true)
#endif
{
    free(object);
}

#if __cplusplus <= 199711L
void operator delete[](void *array)
#else
void operator delete[](void *array) noexcept(true)
#endif
{
    free(array);
}

extern "C" {
    long tzoffset(struct timezone *tz)
    {
        struct timeval now;
        time_t t1, t2 = 0;
        struct tm t;
        
        gettimeofday(&now, tz);
        t1 = now.tv_sec;

#ifdef  HAVE_GMTIME_R
        gmtime_r(&t1, &t);
#else
        t = *gmtime(&t1);
#endif

        t.tm_isdst = 0;
        t2 = mktime(&t);
        return (long)difftime(t1, t2);
    } 
}

#endif


