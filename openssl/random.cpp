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

#ifndef _MSWINDOWS_
#include <fcntl.h>
#endif

namespace ucommon {

void Random::seed(void)
{
    secure::init();
    RAND_poll();
}

bool Random::seed(const uint8_t *buf, size_t size)
{
    secure::init();

    RAND_seed(buf, (int)size);
    return true;
}

size_t Random::key(uint8_t *buf, size_t size)
{
    secure::init();

    if(RAND_bytes(buf, (int)size))
        return size;
    return 0;
}

size_t Random::fill(uint8_t *buf, size_t size)
{
#ifdef  _MSWINDOWS_
    return key(buf, size);
#else
    int fd = open("/dev/urandom", O_RDONLY);
    ssize_t result = 0;

    if(fd > -1) {
        result = read(fd, buf, size);
        close(fd);
    }
    // ugly...would not trust it
    else {
        result = 0;
        while(result++ < (ssize_t)size)
            *(buf++) = rand() & 0xff;
    }

    if(result < 0)
        result = 0;

    return (size_t)result;
#endif
}

bool Random::status(void)
{
    if(RAND_status())
        return true;

    return false;
}

} // namespace ucommon
