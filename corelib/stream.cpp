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

#ifndef UCOMMON_SYSRUNTIME

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/thread.h>
#include <ucommon/socket.h>
#include <ucommon/string.h>
#include <ucommon/shell.h>
#include <ucommon/stream.h>
#include <stdarg.h>

#ifndef _MSWINDOWS_
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#endif

#ifdef  HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace ucommon {
using std::iostream;
using std::streambuf;
using std::ios;

#undef  EOF
#define EOF         std::streambuf::traits_type::eof()
#define IS_EOF(x)    std::streambuf::traits_type::eq_int_type(x, EOF)
#define GET(x)      std::streambuf::traits_type::to_int_type(x)
#define PUT(x)      std::streambuf::traits_type::to_char_type(x)

StreamBuffer::StreamBuffer() :
streambuf(), iostream((streambuf *)this)
{
    bufsize = 0;
    gbuf = pbuf = NULL;
}

int StreamBuffer::uflow()
{
    int ret = underflow();

    if(IS_EOF(ret))
        return EOF;

    if (bufsize != 1)
        gbump(1);

    return ret;
}

void StreamBuffer::allocate(size_t size)
{
    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;

    if(size < 2) {
        bufsize = 1;
        return;
    }

    gbuf = new char[size];
    pbuf = new char[size];
    assert(gbuf != NULL && pbuf != NULL);
    bufsize = size;
    clear();
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
    setb(gbuf, gbuf + size, 0);
#endif
    setg(gbuf, gbuf + size, gbuf + size);
    setp(pbuf, pbuf + size);
}

void StreamBuffer::release(void)
{
    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;
    bufsize = 0;
    clear();
}

int StreamBuffer::sync(void)
{
    if(!bufsize)
        return 0;

    overflow(EOF);
    setg(gbuf, gbuf + bufsize, gbuf + bufsize);
    return 0;
}

tcpstream::tcpstream(const tcpstream &copy) :
StreamBuffer()
{
    so = Socket::create(Socket::family(copy.so), SOCK_STREAM, IPPROTO_TCP);
    timeout = copy.timeout;
}

tcpstream::tcpstream(int family, timeout_t tv) :
StreamBuffer()
{
    so = Socket::create(family, SOCK_STREAM, IPPROTO_TCP);
    timeout = tv;
}

tcpstream::tcpstream(Socket::address& list, unsigned segsize, timeout_t tv) :
StreamBuffer()
{
    so = Socket::create(list.family(), SOCK_STREAM, IPPROTO_TCP);
    timeout = tv;
    open(list);
}

tcpstream::tcpstream(const TCPServer *server, unsigned segsize, timeout_t tv) :
StreamBuffer()
{
    so = server->accept();
    timeout = tv;
    if(so == INVALID_SOCKET) {
        clear(ios::failbit | rdstate());
        return;
    }
    allocate(segsize);
}

tcpstream::~tcpstream()
{
    tcpstream::release();
}

void tcpstream::release(void)
{
    StreamBuffer::release();
    Socket::release(so);
}

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

bool tcpstream::_wait(void)
{
    if(!timeout)
        return true;

    return Socket::wait(so, timeout);
}

ssize_t tcpstream::_read(char *buffer, size_t size)
{
    return Socket::recvfrom(so, buffer, size, MSG_WAITALL);
}

ssize_t tcpstream::_write(const char *buffer, size_t size)
{
    return Socket::sendto(so, buffer, size);
}

int tcpstream::underflow(void)
{
    ssize_t rlen = 1;
    char ch;

    if(bufsize == 1) {
        if(!_wait()) {
            clear(ios::failbit | rdstate());
            return EOF;
        }
        else
            rlen = _read(&ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
                reset();
            return EOF;
        }
        return GET(ch);
    }

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return GET(*gptr());

    rlen = (ssize_t)((gbuf + bufsize) - eback());
    if(!_wait()) {
        clear(ios::failbit | rdstate());
        return EOF;
    }
    else {
        rlen = _read(eback(), rlen);
    }
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
            reset();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return GET(*gptr());
}

int tcpstream::overflow(int c)
{
    char ch;
    ssize_t rlen, req;

    if(bufsize == 1) {
        if(IS_EOF(c))
            return EOF;

        ch = PUT(c);
        rlen = _write((const char *)&ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
                reset();
            return EOF;
        }
        else
            return c;
    }

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = _write(pbase(), req);
        if(rlen < 1) {
            if(rlen < 0)
                reset();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = PUT(c);
        pbump(1);
    }
    return c;
}

void tcpstream::open(Socket::address& list, unsigned mss)
{
    if(bufsize)
        close();    // close if existing is open...

    if(Socket::connectto(so, *list))
        return;

    allocate(mss);
}

void tcpstream::open(const char *host, const char *service, unsigned mss)
{
    if(bufsize)
        close();

    struct addrinfo *list = Socket::query(host, service, SOCK_STREAM, 0);
    if(!list)
        return;

    if(Socket::connectto(so, list)) {
        Socket::release(list);
        return;
    }

    Socket::release(list);
    allocate(mss);
}

void tcpstream::reset(void)
{
    if(!bufsize)
        return;

    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;
    bufsize = 0;
    clear();
    Socket::disconnect(so);
}

void tcpstream::close(void)
{
    if(!bufsize)
        return;

    sync();

    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;
    bufsize = 0;
    clear();
    Socket::disconnect(so);
}

void tcpstream::allocate(unsigned mss)
{
    unsigned size = mss;
    unsigned max = 0;
#ifdef  TCP_MAXSEG
    socklen_t alen = sizeof(max);
#endif

    if(mss == 1)
        goto allocate;

#ifdef  TCP_MAXSEG
    if(mss)
        setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, sizeof(max));
    getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, &alen);
#endif

    if(max && max < mss)
        mss = max;

    if(!mss) {
        if(max)
            mss = max;
        else
            mss = 536;
        goto allocate;
    }

#ifdef  TCP_MAXSEG
    setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

    if(mss < 80)
        mss = 80;

    if(mss * 7 < 64000u)
        bufsize = mss * 7;
    else if(mss * 6 < 64000u)
        bufsize = mss * 6;
    else
        bufsize = mss * 5;

    Socket::sendsize(so, (unsigned)bufsize);
    Socket::recvsize(so, (unsigned)bufsize);

    if(mss < 512)
        Socket::sendwait(so, mss * 4);

allocate:
    StreamBuffer::allocate(size);
}

pipestream::pipestream() :
StreamBuffer()
{
}

pipestream::pipestream(const char *cmd, access_t access, char **args, char **envp, size_t size) :
StreamBuffer()
{
    open(cmd, access, args, envp, size);
}

pipestream::~pipestream()
{
    close();
}

void pipestream::terminate(void)
{
    if(bufsize) {
        shell::cancel(pid);
        close();
    }
}

void pipestream::release(void)
{
    if(gbuf)
        fsys::release(rd);

    if(pbuf)
        fsys::release(wr);

    StreamBuffer::release();
}

void pipestream::allocate(size_t size, access_t mode)
{
    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;

    if(size < 2) {
        bufsize = 1;
        return;
    }

    if(mode == RDONLY || mode == RDWR)
        gbuf = new char[size];
    if(mode == WRONLY || mode == RDWR)
        pbuf = new char[size];
    bufsize = size;
    clear();
    if(mode == RDONLY || mode == RDWR) {
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
        setb(gbuf, gbuf + size, 0);
#endif
        setg(gbuf, gbuf + size, gbuf + size);
    }
    if(mode == WRONLY || mode == RDWR)
        setp(pbuf, pbuf + size);
}

int pipestream::underflow(void)
{
    ssize_t rlen = 1;
    char ch;

    if(!gbuf)
        return EOF;

    if(bufsize == 1) {
        rlen = rd.read(&ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
                close();
            return EOF;
        }
        return GET(ch);
    }

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return GET(*gptr());

    rlen = (ssize_t)((gbuf + bufsize) - eback());
    rlen = rd.read(eback(), rlen);
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
            close();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return GET(*gptr());
}

int pipestream::overflow(int c)
{
    char ch;
    ssize_t rlen, req;

    if(!pbuf)
        return EOF;

    if(bufsize == 1) {
        if(IS_EOF(c))
            return EOF;

        ch = PUT(c);
        rlen = wr.write(&ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
                close();
            return EOF;
        }
        else
            return c;
    }

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = wr.write(pbase(), req);
        if(rlen < 1) {
            if(rlen < 0)
                close();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = PUT(c);
        pbump(1);
    }
    return c;
}

void pipestream::open(const char *path, access_t mode, char **args, char **envp, size_t size)
{
/*#ifdef    RLIMIT_NOFILE
    struct rlimit rlim;

    if(!getrlimit(RLIMIT_NOFILE, &rlim))
        max = rlim.rlim_max;
#endif
*/  close();

    fd_t stdio[3] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
    fd_t input = INVALID_HANDLE_VALUE, output = INVALID_HANDLE_VALUE;

    if(mode == RDONLY || mode == RDWR) {
        if(fsys::pipe(input, stdio[1]))
            return;
        fsys::inherit(input, false);
    }
    else
        stdio[1] = fsys::null();

    if(mode == WRONLY || mode == RDWR) {
        if(fsys::pipe(stdio[0], output)) {
            if(mode == RDWR) {
                fsys::release(stdio[1]);
                fsys::release(input);
            }
            return;
        }
    }
    else
        stdio[0] = fsys::null();

    pid = shell::spawn(path, args, envp, stdio);

    fsys::release(stdio[0]);
    fsys::release(stdio[1]);
    if(pid == INVALID_PID_VALUE) {
        fsys::release(input);
        fsys::release(output);
        input = output = INVALID_HANDLE_VALUE;
    }
    else
        allocate(size, mode);
    rd.assign(input);
    wr.assign(output);
}

int pipestream::close(void)
{
    sync();

    if(bufsize) {
        release();
        return shell::wait(pid);
    }
    return -1;
}

filestream::filestream() :
StreamBuffer()
{
}

filestream::filestream(const filestream& copy) :
StreamBuffer()
{
    if(copy.bufsize)
        fd = copy.fd;
    if(is(fd))
        allocate(copy.bufsize, copy.ac);
}

filestream::filestream(const char *filename, fsys::access_t mode, size_t size) :
StreamBuffer()
{
    open(filename, mode, size);
}

filestream::filestream(const char *filename, unsigned mode, fsys::access_t access, size_t size) :
StreamBuffer()
{
    open(filename, mode, access, size);
}

filestream::~filestream()
{
    close();
}

void filestream::seek(fsys::offset_t offset)
{
    if(bufsize) {
        sync();
        fd.seek(offset);
    }
}

void filestream::rewind(void)
{
    sync();
    if(bufsize) {
        fd.seek(0);
    }
}

void filestream::close(void)
{
    sync();

    if(bufsize)
        fd.close();

    StreamBuffer::release();
}

void filestream::allocate(size_t size, fsys::access_t mode)
{
    if(gbuf)
        delete[] gbuf;

    if(pbuf)
        delete[] pbuf;

    gbuf = pbuf = NULL;
    ac = mode;

    if(size < 2) {
        bufsize = 1;
        return;
    }

    switch (mode) {
    case fsys::RDONLY:
    case fsys::RDWR:
    case fsys::SHARED:
        gbuf = new char[size];
    default:
        break;
    }

    switch (mode) {
    case fsys::WRONLY:
    case fsys::APPEND:
    case fsys::SHARED:
    case fsys::RDWR:
        pbuf = new char[size];
    default:
        break;
    }

    bufsize = size;
    clear();
    switch (mode) {
    case fsys::RDONLY:
    case fsys::RDWR:
    case fsys::SHARED:
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
        setb(gbuf, gbuf + size, 0);
#endif
        setg(gbuf, gbuf + size, gbuf + size);
    default:
        break;
    }

    switch (mode) {
    case fsys::WRONLY:
    case fsys::APPEND:
    case fsys::SHARED:
    case fsys::RDWR:
        setp(pbuf, pbuf + size);
    default:
        break;
    }
}

void filestream::open(const char *fname, unsigned fmode, fsys::access_t access, size_t size)
{
    close();
    fd.open(fname, fmode, access);
    if(is(fd))
        allocate(size, access);
}

void filestream::open(const char *fname, fsys::access_t access, size_t size)
{
    close();
    fd.open(fname, access);
    if(is(fd))
        allocate(size, access);
}

int filestream::underflow(void)
{
    ssize_t rlen = 1;

    if(!gbuf)
        return EOF;

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return GET(*gptr());

    rlen = (ssize_t)((gbuf + bufsize) - eback());
    rlen = fd.read(eback(), rlen);
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
            close();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return GET(*gptr());
}

int filestream::overflow(int c)
{
    ssize_t rlen, req;

    if(!pbuf)
        return EOF;

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = fd.write(pbase(), req);
        if(rlen < 1) {
            if(rlen < 0)
                close();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = PUT(c);
        pbump(1);
    }
    return c;
}

memwriter::memwriter(char *mem, size_t size) :
std::streambuf(), std::ostream(this)
{
    count = size;
    pos = (uint8_t *)mem;
    bp = (uint8_t *)mem;
    zb = true;
    *mem = 0;
    --count;
}

memwriter::memwriter(uint8_t *mem, size_t size) :
std::streambuf(), std::ostream(this)
{
    count = size;
    pos = mem;
    bp = mem;
    zb = false;
}

int memwriter::overflow(int ch)
{
    if(ch == EOF)
        return EOF;

    if(!count || !pos)
        return EOF;

    --count;
    *(pos++) = PUT(ch);
    if(zb)
        *(pos) = 0;
    return ch;
}

memreader::memreader(const char *str) :
std::streambuf(), std::istream(this)
{
    bp = (const uint8_t *)str;
    count = strlen(str);
    pos = (const uint8_t *)str;
}

memreader::memreader(const uint8_t *str, size_t size) :
std::streambuf(), std::istream(this)
{
    bp = str;
    pos = str;
    count = size;
}

int memreader::underflow() 
{
    if(!count || !pos)
        return EOF;
    return GET(*pos);
}

int memreader::uflow()
{
    if(!count || !pos)
        return EOF;
    --count;
    return GET(*(pos++));
} 

bool getline(std::istream& in, char *buffer, size_t size)
{
    *buffer = 0;
    if(!in.good())
        return false;

    in.getline(buffer, size);
    if(!buffer[0])
        return false;

    return true;
}

bool putline(std::ostream& out, char *buffer)
{
    if(!out.good())
        return false;

    out << buffer << std::endl;
    return out.good();
}

std::istream& _stream_operators::input(std::istream& inp, InputProtocol& fmt)
{
    int c = 0;
    while(!c) {
        if(!inp.good())
            c = EOF;
        else
            c = inp.get();

        c = fmt._input(c);
        if(!c)
            continue;
        if(c != EOF)
            inp.putback(c);
    }
    return inp;
}

std::ostream& _stream_operators::print(std::ostream& out, const PrintProtocol& fmt)
{
    if(out.good()) {
        const char *cp = fmt._print();

        if(cp)
            out.write(cp, strlen(cp));
        else
            out << std::endl;
    }
    return out;
}

std::istream& _stream_operators::input(std::istream& inp, string_t& str)
{
    inp.getline(str.c_mem(), str.size());
    return inp;
}

std::ostream& _stream_operators::print(std::ostream& out, const string_t& str)
{
    out.write(str.c_str(), str.len());
    return out;
}

std::istream& _stream_operators::input(std::istream& inp, stringlist_t& list)
{
    size_t size = list.size() - 64;

    char *tmp = (char *)malloc(size);
    while(inp.good()) {
        inp.getline(tmp, size);
        if(!list.filter(tmp, size))
            break;
    }
    free(tmp);
    return inp;
}

std::ostream& _stream_operators::print(std::ostream& out, const stringlist_t& list)
{
    StringPager::iterator sp = list.begin();
    while(is(sp) && out.good()) {
        const char *cp = sp->get();
        size_t size = strlen(cp);
        if(size)
            out.write(cp, size);
        out.put('\n');
        sp.next();
    }
    return out;
}

std::string& _stream_operators::append(std::string& target, String& source)
{
    size_t size = source.count();
    if(!size)
        return target;

    const char *buf = source.c_str();
    std::string tmp(buf, size);
    target += tmp;
    return target;
}

class __LOCAL NullBuffer : public std::streambuf
{
private:
	__DELETE_COPY(NullBuffer);

    NullBuffer();

public:
	int overflow(int c) __OVERRIDE;

    int underflow() __OVERRIDE;

    int uflow() __OVERRIDE;

	static NullBuffer null;
};

NullBuffer::NullBuffer() : streambuf()
{
}

int NullBuffer::overflow(int c)
{
    return c;
}

int NullBuffer::underflow()
{
    return EOF;
}

int NullBuffer::uflow()
{
    return EOF;
}

NullBuffer NullBuffer::null;

std::iostream nullstream(&NullBuffer::null);

} // namespace ucommon

namespace std {
    using namespace ucommon;
    iostream& null = nullstream;
}

#endif

