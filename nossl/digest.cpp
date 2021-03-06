// Copyright (C) 2010-2014 David Sugar, Tycho Softworks.
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

#include "local.h"

namespace ucommon {

bool Digest::has(const char *id)
{
    if(eq_case(id, "md5"))
        return true;

    if(eq_case(id, "sha1") || eq_case(id, "sha") || eq_case(id, "sha160"))
        return true;

    if(eq_case(id, "sha2") || eq_case(id, "sha256") || eq_case(id, "sha384"))
        return true;

    return false;
}

void Digest::set(const char *type)
{
    release();

    if(eq_case(type, "md5")) {
        hashtype = "m";
        context = new MD5_CTX;
        MD5Init((MD5_CTX*)context);
    }
    else if(eq_case(type, "sha") || eq_case(type, "sha1")) {
        hashtype = "1";
        context = new SHA1_CTX;
        SHA1Init((SHA1_CTX*)context);
    }
    else if(eq_case(type, "sha2") || eq_case(type, "sha256")) {
        hashtype = "2";
        context = new sha256_ctx;
        sha256_begin((sha256_ctx *)context);
    }
    else if(eq_case(type, "sha384")) {
        hashtype = "3";
        context = new sha384_ctx;
        sha384_begin((sha384_ctx *)context);
    }
}

void Digest::release(void)
{

    if(context && hashtype) {
        switch(*((char *)hashtype)) {
        case 'm':
            memset(context, 0, sizeof(MD5_CTX));
            delete (MD5_CTX*)context;
            break;
        case '1':
            memset(context, 0, sizeof(SHA1_CTX));
            delete (SHA1_CTX *)context;
            break;
        case '2':
            memset(context, 0, sizeof(sha256_ctx));
            delete (sha256_ctx*)context;
            break;
        case '3':
            memset(context, 0, sizeof(sha384_ctx));
            delete (sha384_ctx*)context;
            break;
        default:
            break;
        }
    }

    bufsize = 0;
    memset(textbuf, 0, sizeof(textbuf));
    hashtype = NULL;
    context = NULL;
}

bool Digest::put(const void *address, size_t size)
{
    if(!context || !hashtype)
        return false;

    switch(*((char *)hashtype)) {
    case 'm':
        MD5Update((MD5_CTX*)context, (const uint8_t *)address, size);
        return true;
    case '1':
        SHA1Update((SHA1_CTX*)context, (const uint8_t *)address, size);
        return true;
    case '2':
        sha256_hash((const unsigned char *)address, size, (sha256_ctx *)context);
        return true;
    case '3':
        sha384_hash((const unsigned char *)address, size, (sha384_ctx *)context);
        return true;
    default:
        return false;
    }
}

void Digest::reset(void)
{
    if(hashtype) {
        switch(*((char *)hashtype)) {
        case 'm':
            if(!context)
                context = new MD5_CTX;
            MD5Init((MD5_CTX*)context);
            break;
        case '1':
            if(!context)
                context = new SHA1_CTX;
            SHA1Init((SHA1_CTX*)context);
            break;
        case '2':
            if(!context)
                context = new sha256_ctx;
            sha256_begin((sha256_ctx *)context);
            break;
        case '3':
            if(!context)
                context = new sha384_ctx;
            sha384_begin((sha384_ctx *)context);
            break;
        default:
            break;
        }
    }
    bufsize = 0;
}

void Digest::recycle(bool bin)
{
    unsigned size = bufsize;

    if(!context || !hashtype)
        return;

    switch(*((char *)hashtype)) {
    case 'm':
        if(!bufsize)
            MD5Final(buffer, (MD5_CTX*)context);
        size = 16;
        MD5Init((MD5_CTX*)context);
        if(bin)
            MD5Update((MD5_CTX*)context, (const uint8_t *)buffer, size);
        else {
            unsigned count = 0;
            while(count < size) {
                snprintf(textbuf + (count * 2), 3,
"%2.2x", buffer[count]);
                ++count;
            }
            MD5Update((MD5_CTX*)context, (const uint8_t *)textbuf, size * 2);
        }
        break;
    case '1':
        if(!bufsize)
            SHA1Final(buffer, (SHA1_CTX*)context);
        size = 20;
        SHA1Init((SHA1_CTX*)context);
        if(bin)
            SHA1Update((SHA1_CTX*)context, (const uint8_t *)buffer, size);
        else {
            unsigned count = 0;
            while(count < size) {
                snprintf(textbuf + (count * 2), 3,
"%2.2x", buffer[count]);
                ++count;
            }
            SHA1Update((SHA1_CTX*)context, (const unsigned
char *)textbuf, size * 2);
        }
        break;
    case '2':
        if(!bufsize)
            sha256_end(buffer, (sha256_ctx *)context);
        size = 32;
        sha256_begin((sha256_ctx *)context);
        if(bin)
            sha256_hash((const unsigned char *)buffer, size, (sha256_ctx *)context);
        else {
            unsigned count = 0;
            while(count < size) {
                snprintf(textbuf + (count * 2), 3, "%2.2x", buffer[count]);
                ++count;
            }
            sha256_hash((const unsigned char *)textbuf, size * 2, (sha256_ctx *)context);
        }
        break;
    case '3':
        if(!bufsize)
            sha384_end(buffer, (sha384_ctx *)context);
        size = 48;
        sha384_begin((sha384_ctx *)context);
        if(bin)
            sha384_hash((const unsigned char *)buffer, size, (sha384_ctx *)context);
        else {
            unsigned count = 0;
            while(count < size) {
                snprintf(textbuf + (count * 2), 3, "%2.2x", buffer[count]);
                ++count;
            }
            sha384_hash((const unsigned char *)textbuf, size * 2, (sha384_ctx *)context);
        }
        break;
    default:
        break;
    }
    bufsize = 0;
}

const uint8_t *Digest::get(void)
{
    unsigned count = 0;

    if(bufsize)
        return buffer;

    if(!context || !hashtype)
        return NULL;

    switch(*((char *)hashtype)) {
    case 'm':
        MD5Final(buffer, (MD5_CTX*)context);
        release();
        bufsize = 16;
        break;
    case '1':
        SHA1Final(buffer, (SHA1_CTX*)context);
        release();
        bufsize = 20;
        break;
    case '2':
        sha256_end(buffer, (sha256_ctx *)context);
        release();
        bufsize = 32;
        break;
    case '3':
        sha384_end(buffer, (sha384_ctx *)context);
        bufsize = 48;
        break;
    default:
        break;
    }

    while(count < bufsize) {
        snprintf(textbuf + (count * 2), 3, "%2.2x", buffer[count]);
        ++count;
    }
    return buffer;
}

} // namespace ucommon
