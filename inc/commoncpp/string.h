// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015-2020 Cherokees of Idaho.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.
//
// This exception applies only to the code released under the name GNU
// Common C++.  If you copy code from other releases into a copy of GNU
// Common C++, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for GNU Common C++, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
//


/**
 * @file commoncpp/string.h
 * @short Common C++ generic string class
 **/

#ifndef COMMONCPP_STRING_H_
#define COMMONCPP_STRING_H_

#ifndef COMMONCPP_CONFIG_H_
#include <commoncpp/config.h>
#endif

namespace ost {

typedef ucommon::String String;

__EXPORT char *lsetField(char *target, size_t size, const char *src, const char fill = 0);
__EXPORT char *rsetField(char *target, size_t size, const char *src, const char fill = 0);
__EXPORT char *newString(const char *src, size_t size = 0);
__EXPORT void delString(char *str);
__EXPORT char *setUpper(char *string, size_t size);
__EXPORT char *setLower(char *string, size_t size);

inline char *setString(char *target, size_t size, const char *str) {
    return String::set(target, size, str);
}

inline char *addString(char *target, size_t size, const char *str) {
    return String::add(target, size, str);
}

inline char *dupString(const char *src, size_t size = 0) {
    return newString(src, size);
}

/*
__EXPORT char *find(const char *cs, char *str, size_t len = 0);
__EXPORT char *rfind(const char *cs, char *str, size_t len = 0);
__EXPORT char *ifind(const char *cs, char *str, size_t len = 0);
__EXPORT char *strip(const char *cs, char *str, size_t len = 0);
__EXPORT size_t strchop(const char *cs, char *str, size_t len = 0);
__EXPORT size_t strtrim(const char *cs, char *str, size_t len = 0);

*/

} // namespace ost

#endif
