// Copyright (C) 2010 David Sugar, Tycho Softworks.
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

#include <ucommon/ucommon.h>

using namespace UCOMMON_NAMESPACE;

static shell::flagopt helpflag('h',"--help",	_TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::numericopt blocks('b', "--blocksize", _TEXT("size of i/o blocks in k (1-x)"), "size k", 1);
static shell::numericopt passes('p', "--passes", _TEXT("passes with randomized data (0-x)"), "count", 1);
static shell::flagopt recursive('R', "--recursive", _TEXT("recursive directory scan"));
static shell::flagopt trunc('t', "--truncate", _TEXT("decompose file by truncation"));
static shell::flagopt verbose('v', "--verbose", _TEXT("show active status"));

static int exit_code = 0;

static void report(const char *path, int code)
{
	const char *err = _TEXT("i/o error");

	switch(code) {
	case EACCES:
	case EPERM:
		err = _TEXT("permission denied");
		break;
	case EROFS:
		err = _TEXT("read-only file system");
		break;
	case ENODEV:
	case ENOENT:
		err = _TEXT("no such file or directory");
		break;
	case ENOTDIR:
		err = _TEXT("not a directory");
		break;
	case ENOTEMPTY:
		err = _TEXT("directory not empty");
		break;
	case ENOSPC:
		err = _TEXT("no space left on device");
		break;
	case EBADF:
	case ENAMETOOLONG:
		err = _TEXT("bad file path");
		break;
	case EBUSY:
	case EINPROGRESS:
		err = _TEXT("file or directory busy");
		break;
	case EINTR:
		err = _TEXT("operation interupted");
		break;
	case ELOOP:
		err = _TEXT("too many sym links");
		break;
	}

	if(!code) {
		if(is(verbose))
			shell::printf(" removed\n");
		return;
	}

	if(is(verbose))
		shell::printf(": %s\n", err);
	else
		shell::errexit(1, "%s: %s\n", path, err);

	exit_code = 1;
}

static void scrubfile(const char *path)
{
	shell::printf("FILE %s\n", path);
}

static void scrubdir(const char *path)
{
	if(is(verbose))
		shell::printf("%s", path);

	report(path, fsys::removeDir(path));
}

static void dirpath(String path, bool top = true)
{
	char filename[128];
	String filepath;
	fsys_t dir(path, fsys::ACCESS_DIRECTORY);
	unsigned count = 0;

	while(is(dir) && fsys::read(dir, filename, sizeof(filename))) {
		if(*filename == '.' && (filename[1] == '.' || !filename[1]))
			continue;

		++count;
		filepath = str(path) + str("/") + str(filename);
		if(fsys::isdir(filepath)) {
			if(is(recursive))
				dirpath(filepath, false);
			else
				scrubdir(filepath);
		}
		else
			scrubfile(filepath);
	}	
	scrubdir(path);
}

extern "C" int main(int argc, char **argv)
{	
	unsigned count = 0;
	char *ep;

	// default bind based on argv0, so we do not have to be explicit...
	// shell::bind("args");
	shell args(argc, argv);

	if(is(helpflag) || is(althelp)) {
		printf("%s\n", _TEXT("Usage: scrub [options] path..."));
		printf("%s\n\n", _TEXT("Echo command line arguments"));
		printf("%s\n", _TEXT("Options:"));
		shell::help();
		printf("\n%s\n", _TEXT("Report bugs to dyfet@gnu.org"));
		exit(0);
	}

	if(!args())
		return 0;

	while(count < args()) {
		if(fsys::isdir(args[count]))
			dirpath(str(args[count++]));
		else
			scrubfile(args[count++]);
	}

	return exit_code;
}
