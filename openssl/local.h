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

#ifdef __APPLE__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <ucommon-config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <ucommon/secure.h>
#include <openssl/ssl.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#ifdef  _MSWINDOWS_
#include <wincrypt.h>
#endif

namespace ucommon {

class __LOCAL __context : public secure
{
public:
    ~__context();

    SSL_CTX *ctx;
};

} // namespace ucommon


