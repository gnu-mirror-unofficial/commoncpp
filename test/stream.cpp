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

#ifndef UCOMMON_SYSRUNTIME
#ifndef DEBUG
#define DEBUG
#endif

#include <ucommon/ucommon.h>

#include <stdio.h>

using namespace ucommon;
using namespace std;

class ThreadOut: public JoinableThread
{
public:
    ThreadOut() : JoinableThread() {};

    void run() {
        Socket::address localhost("127.0.0.1", 9000);
        tcpstream tcp(localhost);
        tcp << "pippo" << endl << ends;
        tcp.close();
    }
};

int main(int argc, char *argv[])
{
    ThreadOut thread;

    char line[200];
    TCPServer sock("127.0.0.1", "9000");
    thread.start();
    if (sock.wait(1000)){
        tcpstream tcp(&sock);
        tcp.getline(line, 200);
        assert(!strcmp(line, "pippo"));
        tcp.close();

        line[0] = 0;
        omemstream memout(line, sizeof(line));
        memout << "test";
        assert(eq(line, "test"));

        std::null << "test";
        String s;
        std::null >> s;

        return 0;
    }
    assert(0);
    return 0;
}
#else

int main(int argc, char **argv)
{
    return 0;
}

#endif
