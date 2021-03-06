// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015-2020 Cherokees of Idaho.
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
#include <ucommon/string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <limits.h>

namespace ucommon {

String::cstring::cstring(size_t size) :
CountedObject()
{
    max = size;
    len = 0;
    text[0] = 0;
}

void String::cstring::fix(void)
{
    text[len] = 0;
}

void String::cstring::clear(size_t offset)
{
    if(offset >= len)
        return;

    text[offset] = 0;
    len = offset;
}

void String::cstring::dec(size_t offset)
{
    if(!len)
        return;

    if(offset >= len) {
        text[0] = 0;
        len = 0;
        fix();
        return;
    }

    text[--len] = 0;
}

void String::cstring::inc(size_t offset)
{
    if(!offset)
        ++offset;

    if(offset >= len) {
        text[0] = 0;
        len = 0;
        fix();
        return;
    }

    memmove(text, text + offset, len - offset);
    len -= offset;
    fix();
}

void String::cstring::add(const char *str)
{
    size_t size = strlen(str);

    if(!size)
        return;

    if(len + size > max)
        size = max - len;

    if(size < 1)
        return;

    memcpy(text + len, str, size);
    len += size;
    fix();
}

void String::cstring::add(char ch)
{
    if(!ch)
        return;

    if(len == max)
        return;

    text[len++] = ch;
    fix();
}

void String::cstring::set(size_t offset, const char *str, size_t size)
{
    assert(str != NULL);

    if(offset >= max || offset > len)
        return;

    if(offset + size > max)
        size = max - offset;

    while(*str && size) {
        text[offset++] = *(str++);
        --size;
    }

    if(offset > len) {
        len = offset;
        text[len] = 0;
    }
}

size_t String::size(void) const
{
    if(!str)
        return 0;

    return str->max;
}

size_t String::len(void) const
{
    if(!str)
        return 0;

    return str->len;
}

void String::cstring::set(const char *str)
{
    assert(str != NULL);

    size_t size = strlen(str);
    if(size > max)
        size = max;

    if(str < text || str > text + len)
        memcpy(text, str, size);
    else if(str != text)
        memmove(text, str, size);
    len = size;
    fix();
}

String::String()
{
    str = NULL;
}

String::String(const char *s, const char *end)
{
    size_t size = 0;

    if(!s)
        s = "";
    else if(!end)
        size = strlen(s);
    else if(end > s)
        size = (size_t)(end - s);
    str = create(size);
    str->retain();
    str->set(s);
}

String::String(const char *s)
{
    size_t size = count(s);
    if(!s)
        s = "";
    str = create(size);
    str->retain();
    str->set(s);
}

String::String(const char *s, size_t size)
{
    if(!s)
        s = "";
    if(!size)
        size = strlen(s);
    str = create(size);
    str->retain();
    str->set(s);
}

String::String(size_t size)
{
    str = create(size);
    str->retain();
}

String::String(size_t size, const char *format, ...)
{
    assert(format != NULL);

    va_list args;
    va_start(args, format);

    str = create(size);
    str->retain();
    vsnprintf(str->text, size + 1, format, args);
    va_end(args);
}

String::String(const String &dup)
{
    str = dup.c_copy();
    if(str)
        str->retain();
}

String::~String()
{
    String::release();
}

String::cstring *String::c_copy(void) const
{
    return str;
}

String String::get(size_t offset, size_t len) const
{
    if(!str || offset >= str->len)
        return String("");

    if(!len)
        len = str->len - offset;
    return String(str->text + offset, len);
}

String::cstring *String::create(size_t size) const
{
    void *mem = ::malloc(size + sizeof(cstring));
    return new(mem) cstring(size);
}

void String::cstring::dealloc(void)
{
    this->cstring::~cstring();
    ::free(this);
}

void String::retain(void)
{
    if(str)
        str->retain();
}

void String::release(void)
{
    if(str)
        str->release();
    str = NULL;
}

char *String::data(void)
{
    if(!str)
        return NULL;

    return str->text;
}

const char *String::c_str(void) const
{
    if(!str)
        return "";

    return str->text;
}

bool String::equal(const char *s) const
{
#ifdef  HAVE_STRCOLL
    const char *mystr = "";

    if(str)
        mystr = str->text;

    if(!s)
        s = "";

    return strcmp(mystr, s) == 0;
#else
    return compare(s) == 0;
#endif
}

int String::compare(const char *s) const
{
    const char *mystr = "";

    if(str)
        mystr = str->text;

    if(!s)
        s = "";

#ifdef  HAVE_STRCOLL
    return strcoll(mystr, s);
#else
    return strcmp(mystr, s);
#endif
}

const char *String::begin(void) const
{
    if(!str)
        return NULL;

    return str->text;
}

const char *String::end(void) const
{
    if(!str)
        return NULL;

    return str->text + str->len;
}

const char *String::chr(char ch) const
{
    assert(ch != 0);

    if(!str)
        return NULL;

    return ::strchr(str->text, ch);
}

const char *String::rchr(char ch) const
{
    assert(ch != 0);

    if(!str)
        return NULL;

    return ::strrchr(str->text, ch);
}

const char *String::skip(const char *clist, size_t offset) const
{
    if(!str || !clist || !*clist || !str->len || offset > str->len)
        return NULL;

    while(offset < str->len) {
        if(!strchr(clist, str->text[offset]))
            return str->text + offset;
        ++offset;
    }
    return NULL;
}

const char *String::rskip(const char *clist, size_t offset) const
{
    if(!str || !clist || !*clist)
        return NULL;

    if(!str->len)
        return NULL;

    if(offset > str->len)
        offset = str->len;

    while(offset--) {
        if(!strchr(clist, str->text[offset]))
            return str->text + offset;
    }
    return NULL;
}

const char *String::rfind(const char *clist, size_t offset) const
{
    if(!str || !clist || !*clist)
        return NULL;

    if(!str->len)
        return str->text;

    if(offset > str->len)
        offset = str->len;

    while(offset--) {
        if(strchr(clist, str->text[offset]))
            return str->text + offset;
    }
    return NULL;
}

void String::chop(const char *clist)
{
    size_t offset;

    if(!str)
        return;

    if(!str->len)
        return;

    offset = str->len;
    while(offset) {
        if(!strchr(clist, str->text[offset - 1]))
            break;
        --offset;
    }

    if(!offset) {
        clear();
        return;
    }

    if(offset == str->len)
        return;

    str->len = offset;
    str->fix();
}

void String::strip(const char *clist)
{
    trim(clist);
    chop(clist);
}

void String::trim(const char *clist)
{
    unsigned offset = 0;

    if(!str)
        return;

    while(offset < str->len) {
        if(!strchr(clist, str->text[offset]))
            break;
        ++offset;
    }

    if(!offset)
        return;

    if(offset == str->len) {
        clear();
        return;
    }

    memmove(str->text, str->text + offset, str->len - offset);
    str->len -= offset;
    str->fix();
}

unsigned String::replace(const char *substring, const char *cp, unsigned flags)
{
    const char *result = "";
    unsigned count = 0;
    size_t cpl = 0;

    if(cp)
        cpl = strlen(cp);

    if(!str || !substring || str->len == 0)
        return 0;

    size_t offset = 0;

    size_t tcl = strlen(substring);

    while(result) {
        const char *text = str->text + offset;
#if defined(_MSWINDOWS_)
        if((flags & 0x01) == INSENSITIVE)
            result = strstr(text, substring);
#elif  defined(HAVE_STRICMP)
        if((flags & 0x01) == INSENSITIVE)
            result = stristr(text, substring);
#else
        if((flags & 0x01) == INSENSITIVE)
            result = strcasestr(text, substring);
#endif
        else
            result = strstr(text, substring);

        if(result) {
            ++count;
            offset = (size_t)(text - str->text);
            cut(offset, tcl);
            if(cpl) {
                paste(offset, cp);
                offset += cpl;
            }
        }
    }
    return count;
}

const char *String::search(const char *substring, unsigned instance, unsigned flags) const
{
    const char *result = "";

    if(!str || !substring || str->len == 0)
        return NULL;

    const char *text = str->text;

    if(!instance)
        ++instance;
    while(instance-- && result) {
#if defined(_MSWINDOWS_)
        if((flags & 0x01) == INSENSITIVE)
            result = strstr(text, substring);
#elif  defined(HAVE_STRICMP)
        if((flags & 0x01) == INSENSITIVE)
            result = stristr(text, substring);
#else
        if((flags & 0x01) == INSENSITIVE)
            result = strcasestr(text, substring);
#endif
        else
            result = strstr(text, substring);

        if(result)
            text = result + strlen(result);
    }
    return result;
}

const char *String::find(const char *clist, size_t offset) const
{
    if(!str || !clist || !*clist || !str->len || offset > str->len)
        return NULL;

    while(offset < str->len) {
        if(strchr(clist, str->text[offset]))
            return str->text + offset;
        ++offset;
    }
    return NULL;
}

bool String::unquote(const char *clist)
{
    assert(clist != NULL);

    char *s;

    if(!str)
        return false;

    s = unquote(str->text, clist);
    if(!s) {
        str->fix();
        return false;
    }

    set(s);
    return true;
}

void String::upper(void)
{
    if(str)
        upper(str->text);
}

void String::lower(void)
{
    if(str)
        lower(str->text);
}

void String::erase(void)
{
    if(str) {
        memset(str->text, 0, str->max);
        str->fix();
    }
}

size_t String::offset(const char *s) const
{
    if(!str || !s)
        return npos;

    if(s < str->text || s > str->text + str->max)
        return npos;

    if((size_t)(s - str->text) > str->len)
        return str->len;

    return (size_t)(s - str->text);
}

size_t String::count(void) const
{
    if(!str)
        return 0;
    return str->len;
}

size_t String::ccount(const char *clist) const
{
    if(!str)
        return 0;

    return ccount(str->text, clist);
}

size_t String::printf(const char *format, ...)
{
    assert(format != NULL);

    va_list args;
    va_start(args, format);
    if(str) {
        vsnprintf(str->text, str->max + 1, format, args);
        str->len = strlen(str->text);
        str->fix();
    }
    va_end(args);
    return len();
}

size_t String::vprintf(const char *format, va_list args)
{
    assert(format != NULL);

    if(str) {
        vsnprintf(str->text, str->max + 1, format, args);
        str->len = strlen(str->text);
        str->fix();
    }
    return len();
}

#if !defined(_MSC_VER)
int String::vscanf(const char *format, va_list args)
{
    assert(format != NULL);

    if(str)
        return vsscanf(str->text, format, args);
    return -1;
}

int String::scanf(const char *format, ...)
{
    assert(format != NULL);

    va_list args;
    int rtn = -1;

    va_start(args, format);
    if(str)
        rtn = vsscanf(str->text, format, args);
    va_end(args);
    return rtn;
}
#else
int String::vscanf(const char *format, va_list args)
{
    return 0;
}

int String::scanf(const char *format, ...)
{
    return 0;
}
#endif

void String::rsplit(const char *s)
{
    if(!str || !s || s <= str->text || s > str->text + str->len)
        return;

    str->set(s);
}

void String::rsplit(size_t pos)
{
    if(!str || pos > str->len || !pos)
        return;

    str->set(str->text + pos);
}

void String::split(const char *s)
{
    size_t pos;

    if(!s || !*s || !str)
        return;

    if(s < str->text || s >= str->text + str->len)
        return;

    pos = s - str->text;
    str->text[pos] = 0;
    str->fix();
}

void String::split(size_t pos)
{
    if(!str || pos >= str->len)
        return;

    str->text[pos] = 0;
    str->fix();
}

void String::fill(size_t size, char fill)
{
    if(!str) {
        str = create(size);
        str->retain();
    }
    while(str->len < str->max && size--)
        str->text[str->len++] = fill;
    str->fix();
}

void String::set(size_t offset, const char *s, size_t size)
{
    if(!s || !*s || !str)
        return;

    if(!size)
        size = strlen(s);

    if(str)
        str->set(offset, s, size);
}

void String::set(const char *s, char overflow, size_t offset, size_t size)
{
    size_t len = count(s);

    if(!s || !*s || !str)
        return;

    if(offset >= str->max)
        return;

    if(!size || size > str->max - offset)
        size = str->max - offset;

    if(len <= size) {
        set(offset, s, size);
        return;
    }

    set(offset, s, size);

    if(len > size && overflow)
        str->text[offset + size - 1] = overflow;
}

void String::rset(const char *s, char overflow, size_t offset, size_t size)
{
    size_t len = count(s);

    if(!s || !*s || !str)
        return;

    if(offset >= str->max)
        return;

    if(!size || size > str->max - offset)
        size = str->max - offset;

    if(len > size)
        s += len - size;
    set(offset, s, size);
    if(overflow && len > size)
        str->text[offset] = overflow;
}

void String::set(const char *s)
{
    size_t len;

    if(!s)
        s = "";

    if(!str) {
        len = strlen(s);
        str = create(len);
        str->retain();
    }

    str->set(s);
}

void String::paste(size_t offset, const char *cp, size_t size)
{
    if(!cp)
        return;

    if(!size)
        size = strlen(cp);

    if(!size)
        return;

    if(!str) {
        str = create(size);
        String::set(str->text, ++size, cp);
        str->len = --size;
        str->fix();
        return;
    }

    cow(size);

    if(offset >= str->len)
        String::set(str->text + str->len, size + 1, cp);
    else {
        memmove(str->text + offset + size, str->text + offset, str->len - offset);
        memmove(str->text + offset, cp, size);
    }
    str->len += size;
    str->fix();
}

void String::cut(size_t offset, size_t size)
{
    if(!str || offset >= str->len)
        return;

    if(!size)
        size = str->len;

    if(offset + size >= str->len) {
        str->len = offset;
        str->fix();
        return;
    }

    memmove(str->text + offset, str->text + offset + size, str->len - offset - size);
    str->len -= size;
    str->fix();
}

void String::paste(char *text, size_t max, size_t offset, const char *cp, size_t size)
{
    if(!cp || !text)
        return;

    if(!size)
        size = strlen(cp);

    if(!size)
        return;

    size_t len = strlen(text);
    if(len <= max)
        return;

    if(len + size >= max)
        size = max - len;

    if(offset >= len)
        String::set(text + len, size + 1, cp);
    else {
        memmove(text + offset + size, text + offset, len - offset);
        memmove(text + offset, cp, size);
    }
}

void String::cut(char *text, size_t offset, size_t size)
{
    if(!text)
        return;

    size_t len = strlen(text);
    if(!len || offset >= len)
        return;

    if(offset + size >= len) {
        text[offset] = 0;
        return;
    }

    memmove(text + offset, text + offset + size, len - offset - size);
    len -= size;
    text[len] = 0;
}

bool String::resize(size_t size)
{
    if(!size) {
        release();
        str = NULL;
        return true;
    }

    if(!str) {
        str = create(size);
        str->retain();
    }
    else if(str->is_copied() || str->max < size) {
        str->release();
        str = create(size);
        str->retain();
    }
    return true;
}

void String::clear(size_t offset)
{
    if(!str)
        return;

    str->clear(offset);
}

void String::clear(void)
{
    if(str)
        str->set("");
}

void String::cow(size_t size)
{
    if(str)
        size += str->len;

    if(!size)
        return;

    if(!str || !str->max || str->is_copied() || size > str->max) {
        cstring *s = create(size);
        if (!s)
            return;

		if (str) {
			s->len = str->len;
			String::set(s->text, s->max + 1, str->text);
		}
		else
			s->len = 0;
        s->retain();
		if (str)
			str->release();
        str = s;
    }
}

void String::add(char ch)
{
    char buf[2];

    if(ch == 0)
        return;

    buf[0] = ch;
    buf[1] = 0;

    if(!str) {
        set(buf);
        return;
    }

    cow(1);
    str->add(buf);
}

void String::add(const char *s)
{
    if(!s || !*s)
        return;

    if(!str) {
        set(s);
        return;
    }

    cow(strlen(s));
    str->add(s);
}

char String::at(int offset) const
{
    if(!str)
        return 0;

    if(offset >= (int)str->len)
        return 0;

    if(offset > -1)
        return str->text[offset];

    if((size_t)(-offset) >= str->len)
        return str->text[0];

    return str->text[(int)(str->len) + offset];
}

String String::operator()(int offset, size_t len) const
{
    const char *cp = operator()(offset);
    if(!cp)
        cp = "";

    if(!len)
        len = strlen(cp);

    return String(cp, len);
}

const char *String::operator()(int offset) const
{
    if(!str)
        return NULL;

    if(offset >= (int)str->len)
        return NULL;

    if(offset > -1)
        return str->text + offset;

    if((size_t)(-offset) >= str->len)
        return str->text;

    return str->text + str->len + offset;
}

const char String::operator[](int offset) const
{
    if(!str)
        return 0;

    if(offset >= (int)str->len)
        return 0;

    if(offset > -1)
        return str->text[offset];

    if((size_t)(-offset) >= str->len)
        return *str->text;

    return str->text[str->len + offset];
}

String &String::operator++()
{
    if(str)
        str->inc(1);
    return *this;
}

String &String::operator--()
{
    if(str)
        str->dec(1);
    return *this;
}

String &String::operator+=(size_t offset)
{
    if(str)
        str->inc(offset);
    return *this;
}

String &String::operator-=(size_t offset)
{
    if(str)
        str->dec(offset);
    return *this;
}

bool String::operator*=(const char *substr)
{
    if(search(substr))
        return true;

    return false;
}

String &String::operator^=(const char *s)
{
    release();
    set(s);
    return *this;
}

String &String::operator=(const char *s)
{
    release();
    set(s);
    return *this;
}

String &String::operator|=(const char *s)
{
    set(s);
    return *this;
}

String &String::operator&=(const char *s)
{
    set(s);
    return *this;
}

String &String::operator^=(const String &s)
{
    release();
    set(s.c_str());
    return *this;
}

String &String::operator=(const String &s)
{
    if(str == s.str)
        return *this;

    if(s.str)
        s.str->retain();

    if(str)
        str->release();

    str = s.str;
    return *this;
}

bool String::full(void) const
{
    if(!str)
        return false;

    if(str->len == str->max)
        return true;

    return false;
}

bool String::operator!() const
{
    bool rtn = false;
    if(!str)
        return true;

    if(!str->len)
        rtn = true;

    str->fix();
    return rtn;
}

String::operator bool() const
{
    bool rtn = false;

    if(!str)
        return false;

    if(str->len)
        rtn = true;
    str->fix();
    return rtn;
}

bool String::operator==(const char *s) const
{
    return (compare(s) == 0);
}

bool String::operator!=(const char *s) const
{
    return (compare(s) != 0);
}

bool String::operator<(const char *s) const
{
    return (compare(s) < 0);
}

bool String::operator<=(const char *s) const
{
    return (compare(s) <= 0);
}

bool String::operator>(const char *s) const
{
    return (compare(s) > 0);
}

bool String::operator>=(const char *s) const
{
    return (compare(s) >= 0);
}

String &String::operator&(const char *s)
{
    add(s);
    return *this;
}

String &String::operator|(const char *s)
{
    if(!s || !*s)
        return *this;

    if(!str) {
        set(s);
        return *this;
    }

    str->add(s);
    return *this;
}

const String String::operator+(const char *s) const
{
    String tmp;
    if(str && str->text[0])
        tmp.set(str->text);

    if(s && *s)
        tmp.add(s);

    return tmp;
}

String &String::operator+=(const char *s)
{
    if(!s || !*s)
        return *this;

    add(s);
    return *this;
}

memstring::memstring(void *mem, size_t size)
{
    assert(mem != NULL);
    assert(size > 0);

    str = new(mem) cstring(size);
    str->set("");
}

memstring::~memstring()
{
    str = NULL;
}

memstring *memstring::create(size_t size)
{
    assert(size > 0);

    void *mem = ::malloc(size + sizeof(memstring) + sizeof(cstring));
    return new(mem) memstring((caddr_t)mem + sizeof(memstring), size);
}

memstring *memstring::create(MemoryProtocol *mpager, size_t size)
{
    assert(size > 0);

    void *mem = mpager->alloc(size + sizeof(memstring) + sizeof(cstring));
    return new(mem) memstring((caddr_t)mem + sizeof(memstring), size);
}

void memstring::release(void)
{
    str = NULL;
}

bool memstring::resize(size_t size)
{
    return false;
}

void memstring::cow(size_t adj)
{
}

char *String::token(char *text, char **token, const char *clist, const char *quote, const char *eol)
{
    char *result;

    if(!eol)
        eol = "";

    if(!token || !clist)
        return NULL;

    if(!*token)
        *token = text;

    if(!**token) {
        *token = text;
        return NULL;
    }

    while(**token && strchr(clist, **token))
        ++*token;

    result = *token;

    if(*result && *eol && NULL != (eol = strchr(eol, *result))) {
        if(eol[0] != eol[1] || *result == eol[1]) {
            *token = text;
            return NULL;
        }
    }

    if(!*result) {
        *token = text;
        return NULL;
    }

    while(quote && *quote && *result != *quote) {
        quote += 2;
    }

    if(quote && *quote) {
        ++result;
        ++quote;
        *token = strchr(result, *quote);
        if(!*token)
            *token = result + strlen(result);
        else {
            **token = 0;
            ++(*token);
        }
        return result;
    }

    while(**token && !strchr(clist, **token))
        ++(*token);

    if(**token) {
        **token = 0;
        ++(*token);
    }

    return result;
}

String::cstring *memstring::c_copy(void) const
{
    cstring *tmp = String::create(str->max);
    tmp->set(str->text);
    return tmp;
}

void String::fix(String &s)
{
    if(s.str) {
        s.str->len = strlen(s.str->text);
        s.str->fix();
    }
}

void String::swap(String &s1, String &s2)
{
    String::cstring *s = s1.str;
    s1.str = s2.str;
    s2.str = s;
}

char *String::dup(const char *cp)
{
    char *mem;

    if(!cp)
        return NULL;

    size_t len = strlen(cp) + 1;
    mem = (char *)malloc(len);
    if(!mem)
        __THROW_ALLOC();
    String::set(mem, len, cp);
    return mem;
}

char *String::left(const char *cp, size_t size)
{
    char *mem;

    if(!cp)
        return NULL;

    if(!size)
        size = strlen(cp);

    mem = (char *)malloc(++size);
    if(!mem)
        __THROW_ALLOC();
    String::set(mem, size, cp);
    return mem;
}

const char *String::pos(const char *cp, ssize_t offset)
{
    if(!cp)
        return NULL;

    size_t len = strlen(cp);
    if(!len)
        return cp;

    if(offset >= 0) {
        if((size_t)offset > len)
            offset = (ssize_t)len;
        return cp + offset;
    }

    offset = -offset;
    if((size_t)offset >= len)
        return cp;

    return cp + len - offset;
}

size_t String::count(const char *cp)
{
    if(!cp)
        return 0;

    return strlen(cp);
}

const char *String::find(const char *str, const char *key, const char *delim)
{
    size_t l1 = strlen(str);
    size_t l2 = strlen(key);

    if(!delim[0])
        delim = NULL;

    while(l1 >= l2) {
        if(!strncmp(key, str, l2)) {
            if(l1 == l2 || !delim || strchr(delim, str[l2]))
                return str;
        }
        if(!delim) {
            ++str;
            --l1;
            continue;
        }
        while(l1 >= l2 && !strchr(delim, *str)) {
            ++str;
            --l1;
        }
        while(l1 >= l2 && strchr(delim, *str)) {
            ++str;
            --l1;
        }
    }
    return NULL;
}

const char *String::ifind(const char *str, const char *key, const char *delim)
{
    size_t l1 = strlen(str);
    size_t l2 = strlen(key);

    if(!delim[0])
        delim = NULL;

    while(l1 >= l2) {
        if(!strnicmp(key, str, l2)) {
            if(l1 == l2 || !delim || strchr(delim, str[l2]))
                return str;
        }
        if(!delim) {
            ++str;
            --l1;
            continue;
        }
        while(l1 >= l2 && !strchr(delim, *str)) {
            ++str;
            --l1;
        }
        while(l1 >= l2 && strchr(delim, *str)) {
            ++str;
            --l1;
        }
    }
    return NULL;
}

char *String::set(char *str, size_t size, const char *s, size_t len)
{
    if(!str)
        return NULL;

    if(size < 2)
        return str;

    if(!s)
        s = "";

    size_t l = strlen(s);
    if(l >= size)
        l = size - 1;

    if(l > len)
        l = len;

    if(!l) {
        *str = 0;
        return str;
    }

    memmove(str, s, l);
    str[l] = 0;
    return str;
}

char *String::rset(char *str, size_t size, const char *s)
{
    size_t len = count(s);
    if(len > size)
        s += len - size;
    return set(str, size, s);
}

char *String::set(char *str, size_t size, const char *s)
{
    if(!str)
        return NULL;

    if(size < 2)
        return str;

    if(!s)
        s = "";

    size_t l = strlen(s);

    if(l >= size)
        l = size - 1;

    if(!l) {
        *str = 0;
        return str;
    }

    memmove(str, s, l);
    str[l] = 0;
    return str;
}

char *String::add(char *str, size_t size, const char *s, size_t len)
{
    if(!str)
        return NULL;

    if(!s)
        return str;

    size_t l = strlen(s);
    size_t o = strlen(str);

    if(o >= (size - 1))
        return str;
    set(str + o, size - o, s, l);
    return str;
}

char *String::add(char *str, size_t size, const char *s)
{
    if(!str)
        return NULL;

    if(!s)
        return str;

    size_t o = strlen(str);

    if(o >= (size - 1))
        return str;

    set(str + o, size - o, s);
    return str;
}

char *String::trim(char *str, const char *clist)
{
    if(!str)
        return NULL;

    if(!clist)
        return str;

    while(*str && strchr(clist, *str))
        ++str;

    return str;
}

char *String::chop(char *str, const char *clist)
{
    if(!str)
        return NULL;

    if(!clist)
        return str;

    size_t offset = strlen(str);
    while(offset && strchr(clist, str[offset - 1]))
        str[--offset] = 0;
    return str;
}

char *String::strip(char *str, const char *clist)
{
    str = trim(str, clist);
    chop(str, clist);
    return str;
}

bool String::check(const char *str, size_t max, size_t min)
{
    size_t count = 0;

    if(!str)
        return false;

    while(*str) {
        if(++count > max)
            return false;
        ++str;
    }
    if(count < min)
        return false;
    return true;
}

void String::upper(char *str)
{
    while(str && *str) {
        *str = toupper(*str);
        ++str;
    }
}

void String::lower(char *str)
{
    while(str && *str) {
        *str = tolower(*str);
        ++str;
    }
}

void String::erase(char *str)
{
    if(!str || *str == 0)
        return;

    memset(str, 0, strlen(str));
}

unsigned String::ccount(const char *str, const char *clist)
{
    unsigned count = 0;
    while(str && *str) {
        if(strchr(clist, *(str++)))
            ++count;
    }
    return count;
}

char *String::skip(char *str, const char *clist)
{
    if(!str || !clist)
        return NULL;

    while(*str && strchr(clist, *str))
        ++str;

    if(*str)
        return str;

    return NULL;
}

char *String::rskip(char *str, const char *clist)
{
    size_t len = count(str);

    if(!len || !clist)
        return NULL;

    while(len > 0) {
        if(!strchr(clist, str[--len]))
            return str;
    }
    return NULL;
}

size_t String::seek(char *str, const char *clist)
{
    size_t pos = 0;

    if(!str)
        return 0;

    if(!clist)
        return strlen(str);

    while(str[pos]) {
        if(strchr(clist, str[pos]))
            return pos;
        ++pos;
    }
    return pos;
}

char *String::find(char *str, const char *clist)
{
    if(!str)
        return NULL;

    if(!clist)
        return str;

    while(str && *str) {
        if(strchr(clist, *str))
            return str;
    }
    return NULL;
}

char *String::rfind(char *str, const char *clist)
{
    if(!str)
        return NULL;

    if(!clist)
        return str + strlen(str);

    char *s = str + strlen(str);

    while(s > str) {
        if(strchr(clist, *(--s)))
            return s;
    }
    return NULL;
}

bool String::eq_case(const char *s1, const char *s2)
{
    if(!s1)
        s1 = "";

    if(!s2)
        s2 = "";

#ifdef  HAVE_STRICMP
    return stricmp(s1, s2) == 0;
#else
    return strcasecmp(s1, s2) == 0;
#endif
}

bool String::eq_case(const char *s1, const char *s2, size_t size)
{
    if(!s1)
        s1 = "";

    if(!s2)
        s2 = "";

#ifdef  HAVE_STRICMP
    return strnicmp(s1, s2, size) == 0;
#else
    return strncasecmp(s1, s2, size) == 0;
#endif
}

bool String::equal(const char *s1, const char *s2)
{
    if(!s1)
        s1 = "";
    if(!s2)
        s2 = "";

    return strcmp(s1, s2) == 0;
}

bool String::equal(const char *s1, const char *s2, size_t size)
{
    if(!s1)
        s1 = "";
    if(!s2)
        s2 = "";

    return strncmp(s1, s2, size) == 0;
}

int String::compare(const char *s1, const char *s2)
{
    if(!s1)
        s1 = "";
    if(!s2)
        s2 = "";

#ifdef  HAVE_STRCOLL
    return strcoll(s1, s2);
#else
    return strcmp(s1, s2);
#endif
}


char *String::unquote(char *str, const char *clist)
{
    assert(clist != NULL);

    size_t len = count(str);
    if(!len || !str)
        return NULL;

    while(clist[0]) {
        if(*str == clist[0] && str[len - 1] == clist[1]) {
            str[len - 1] = 0;
            return ++str;
        }
        clist += 2;
    }
    return str;
}

char *String::fill(char *str, size_t size, char fill)
{
    if(!str)
        return NULL;

    memset(str, fill, size - 1);
    str[size - 1] = 0;
    return str;
}

static int hexcode(char ch)
{
    ch = toupper(ch);
    if(ch >= '0' && ch <= '9')
        return ch - '0';
    else if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;  // error flag
}

size_t String::hexcount(const char *str, bool ws)
{
    size_t count = 0;

    while(str && *str) {
        if(ws && isspace(*str)) {
            ++str;
            continue;
        }
        if(hexcode(str[0]) < 0 || hexcode(str[1]) < 0)
            break;
        str += 2;
        ++count;
    }

    return count;
}

size_t String::hexsize(const char *format)
{
    size_t count = 0;
    char *ep;
    unsigned skip;

    while(format && *format) {
        while(*format && !isdigit(*format)) {
            ++format;
            ++count;
        }
        if(isdigit(*format)) {
            skip = (unsigned)strtol(format, &ep, 10);
            format = ep;
            count += skip * 2;
        }
    }
    return count;
}

String String::hex(const uint8_t *binary, size_t size)
{
    String out(size * 2);
    char *buf = out.data();
    while(size--) {
        snprintf(buf, 3, "%02x", *(binary++));
        buf += 2;
    }
    return out;
}

size_t String::hexdump(const uint8_t *binary, char *string, const char *format)
{
    size_t count = 0;
    char *ep;
    unsigned skip;

    while(format && *format) {
        while(*format && !isdigit(*format)) {
            *(string++) = *(format++);
            ++count;
        }
        if(isdigit(*format)) {
            skip = (unsigned)strtol(format, &ep, 10);
            format = ep;
            count += skip * 2;
            while(skip--) {
                snprintf(string, 3, "%02x", *(binary++));
                string += 2;
            }
        }
    }
    *string = 0;
    return count;
}

size_t String::hex2bin(const char *str, uint8_t *bin, size_t max, bool ws)
{
    size_t count = 0;
    size_t out = 0;
    int hi, lo;
    while(str && *str) {
        if(ws && isspace(*str)) {
            ++count;
            ++str;
            continue;
        }
        hi = hexcode(str[0]);
        lo = hexcode(str[1]);
        if(hi < 0 || lo < 0)
            break;
        *(bin++) = (hi << 4) | lo;
        str += 2;
        count += 2;
        if(++out > max)
            break;
    }
    return count;
}

size_t String::hexpack(uint8_t *binary, const char *string, const char *format)
{
    size_t count = 0;
    char *ep;
    unsigned skip;

    while(format && *format) {
        while(*format && !isdigit(*format)) {
            if(*(string++) != *(format++))
                return count;
            ++count;
        }
        if(isdigit(*format)) {
            skip = (unsigned)strtol(format, &ep, 10);
            format = ep;
            count += skip * 2;
            while(skip--) {
                *(binary++) = hexcode(string[0]) * 16 + hexcode(string[1]);
                string += 2;
            }
        }
    }
    return count;
}

String &String::operator%(unsigned short& value)
{
    unsigned long temp = USHRT_MAX + 1;
    char *ep;
    if(!str || !str->text[0])
        return *this;

    value = 0;
    temp = strtoul(str->text, &ep, 0);
    if(temp > USHRT_MAX)
        goto failed;

    value = (unsigned short)temp;

    if(ep)
        set(ep);
    else
        set("");

failed:
    return *this;
}

String &String::operator%(short& value)
{
    long temp = SHRT_MAX + 1;
    char *ep;
    if(!str || !str->text[0])
        return *this;

    value = 0;
    temp = strtol(str->text, &ep, 0);
    if(temp < 0 && temp < SHRT_MIN)
        goto failed;

    if(temp > SHRT_MAX)
        goto failed;

    value = (short)temp;

    if(ep)
        set(ep);
    else
        set("");

failed:
    return *this;
}

String &String::operator%(long& value)
{
    value = 0;
    char *ep;
    if(!str || !str->text[0])
        return *this;

    value = strtol(str->text, &ep, 0);
    if(ep)
        set(ep);
    else
        set("");

    return *this;
}

String &String::operator%(unsigned long& value)
{
    value = 0;
    char *ep;
    if(!str || !str->text[0])
        return *this;

    value = strtoul(str->text, &ep, 0);
    if(ep)
        set(ep);
    else
        set("");

    return *this;
}

String &String::operator%(double& value)
{
    value = 0;
    char *ep;
    if(!str || !str->text[0])
        return *this;

    value = strtod(str->text, &ep);
    if(ep)
        set(ep);
    else
        set("");

    return *this;
}

String &String::operator%(const char *get)
{
    if(!str || !str->text[0] || !get)
        return *this;

    size_t len = strlen(get);
    const char *cp = str->text;

    while(isspace(*cp))
        ++cp;

    if(eq(cp, get, len))
        set(cp + len);
    else if(cp != str->text)
        set(cp);

    return *this;
}

static const uint8_t alphabet[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String String::b64(const uint8_t *bin, size_t size)
{
    size_t dsize = (size * 4 / 3) + 1;
    String out(dsize);

    b64encode(out.data(), bin, size);
    return out;
}

size_t String::b64encode(char *dest, const uint8_t *bin, size_t size, size_t dsize)
{
    assert(dest != NULL && bin != NULL);

    size_t count = 0;

    if(!dsize)
        dsize = b64size(size);

    if (!dsize || !size)
        goto end;

    unsigned bits;

    while(size >= 3 && dsize > 4) {
        bits = (((unsigned)bin[0])<<16) | (((unsigned)bin[1])<<8)
            | ((unsigned)bin[2]);
        bin += 3;
        size -= 3;
        count += 3;
        *(dest++) = alphabet[bits >> 18];
        *(dest++) = alphabet[(bits >> 12) & 0x3f];
        *(dest++) = alphabet[(bits >> 6) & 0x3f];
        *(dest++) = alphabet[bits & 0x3f];
        dsize -= 4;
    }

    if (size && dsize > 4) {
        bits = ((unsigned)bin[0])<<16;
        *(dest++) = alphabet[bits >> 18];
        ++count;
        if (size == 1) {
            *(dest++) = alphabet[(bits >> 12) & 0x3f];
            *(dest++) = '=';
        }
        else {
            ++count;
            bits |= ((unsigned)bin[1])<<8;
            *(dest++) = alphabet[(bits >> 12) & 0x3f];
            *(dest++) = alphabet[(bits >> 6) & 0x3f];
        }
        *(dest++) = '=';
    }

end:
    *dest = 0;
    return count;
}

size_t String::b64size(size_t size)
{
    return (size * 4 / 3) + 4;
}

size_t String::b64count(const char *src, bool ws)
{
    char decoder[256];
    unsigned long bits;
    uint8_t c;
    unsigned i;
    size_t count = 0;

    for (i = 0; i < 256; ++i)
        decoder[i] = 64;

    for (i = 0; i < 64 ; ++i)
        decoder[alphabet[i]] = i;

    bits = 1;

    while(*src) {
        if(isspace(*src)) {
            if(ws) {
                ++src;
                continue;
            }
            break;
        }
        c = (uint8_t)(*(src++));
        if (c == '=')
            break;
        // end on invalid chars
        if (decoder[c] == 64) {
            break;
        }
        bits = (bits << 6) + decoder[c];
        if (bits & 0x1000000) {
            bits = 1;
            count += 3;
        }
    }

    if (bits & 0x40000)
        count += 2;
    else if (bits & 0x1000)
        ++count;

    return count;
}

size_t String::b64decode(uint8_t *dest, const char *src, size_t size, bool ws)
{
    char decoder[256];
    unsigned long bits;
    uint8_t c;
    unsigned i;
    size_t count = 0;

    for (i = 0; i < 256; ++i)
        decoder[i] = 64;

    for (i = 0; i < 64 ; ++i)
        decoder[alphabet[i]] = i;

    bits = 1;

    while(*src) {
        if(isspace(*src)) {
            if(ws) {
                ++count;
                ++src;
                continue;
            }
            break;
        }
        c = (uint8_t)(*(src++));
        if (c == '=') {
            ++count;
            if(*src == '=')
                ++count;
            break;
        }
        // end on invalid chars
        if (decoder[c] == 64) {
            break;
        }
        ++count;
        bits = (bits << 6) + decoder[c];
        if (bits & 0x1000000) {
            if (size < 3) {
                bits = 1;
                break;
            }
            *(dest++) = (uint8_t)((bits >> 16) & 0xff);
            *(dest++) = (uint8_t)((bits >> 8) & 0xff);
            *(dest++) = (uint8_t)((bits & 0xff));
            bits = 1;
            size -= 3;
        }
    }
    if (bits & 0x40000) {
        if (size >= 2) {
            *(dest++) = (uint8_t)((bits >> 10) & 0xff);
            *(dest++) = (uint8_t)((bits >> 2) & 0xff);
        }
    }
    else if ((bits & 0x1000) && size) {
        *(dest++) = (uint8_t)((bits >> 4) & 0xff);
    }
    return count;
}

#define CRC24_INIT 0xb704ceL
#define CRC24_POLY 0x1864cfbL

uint32_t String::crc24(uint8_t *binary, size_t size)
{
    uint32_t crc = CRC24_INIT;
    unsigned i;

    while (size--) {
        crc ^= (*binary++) << 16;
        for (i = 0; i < 8; i++) {
            crc <<= 1;
            if (crc & 0x1000000)
                crc ^= CRC24_POLY;
        }
    }
    return crc & 0xffffffL;
}

uint16_t String::crc16(uint8_t *binary, size_t size)
{
    uint16_t crc = 0xffff;
    unsigned i;

    while (size--) {
        crc ^= (*binary++);
        for (i = 0; i < 8; i++) {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xa001;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

} // namespace ucommon
