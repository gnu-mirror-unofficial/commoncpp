// Copyright (C) 2013-2014 David Sugar, Tycho Softworks.
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

#include <ucommon/secure.h>
#include <sys/stat.h>

using namespace ucommon;

static shell::flagopt helpflag('h',"--help",    _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::flagopt reqcert('C', NULL, _TEXT("requires certificate"));
static shell::flagopt verified('V', NULL, _TEXT("requires verification"));
static shell::numericopt port('p', "--port", _TEXT("port to use"), "port", 0);

int main(int argc, char **argv)
{
    shell::bind("urlout");
    shell args(argc, argv);

    if(is(helpflag) || is(althelp) || !args() || args() > 1) {
        printf("%s\n", _TEXT("Usage: urlout [options] url-path"));
        printf("\n%s\n", _TEXT("Options:"));
        shell::help();
        printf("\n%s\n", _TEXT("Report bugs to dyfet@gnu.org"));
        return 0;
    }

    bool url_secure = false;
    const char *url = args[0];
    const char *proto = "80";
    secure::client_t ctx = NULL;
    String svc;

    if(eq(url, "https://", 8)) {
        url_secure = true;
        url += 8;
    }
    else if(eq(url, "http://", 7)) {
        url += 7;
    }
    else
        url_secure = true;

    string_t host = url;
    string_t path = strdup(url);

    if(url_secure && secure::init()) {
        proto = "443";
        ctx = secure::client();
        if(ctx->err() != secure::OK)
            shell::errexit(2, _TEXT("%s: no certificates found"), argv[0]);
    }

    if(is(port))
        svc = str(*port);
    else
        svc = proto;

    const char *find = path.find("/");
    if(find)
        path.rsplit(find);
    else
        path = "/";

    host.split(host.find("/"));

    sstream web(ctx);
    web.open(host, svc);

    if(!web.is_open())
        shell::errexit(1, _TEXT("%s: failed to access"), url);

    if(is(verified) && !web.is_verified())
        shell::errexit(8, _TEXT("%s: unverified host"), url);

    if(is(reqcert) && !web.is_certificate())
        shell::errexit(9, _TEXT("%s: no certificate"), url);

    web << "GET /\r\n\r\n";
    web.flush();

    std::cout << web.rdbuf();
}
