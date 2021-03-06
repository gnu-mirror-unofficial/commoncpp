// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015-2020 Cherokees of Idaho.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#ifndef _MSC_VER
#include <sys/stat.h>
#endif

// needed for GNU/LINUX glibc otherwise pread/pwrite wont work

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

/*
 * on old glibc's, this has to be
 * defined explicitly
 */
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <ucommon-config.h>

#include <commoncpp/config.h>
#include <commoncpp/export.h>
#include <commoncpp/exception.h>
#include <commoncpp/thread.h>
#include <commoncpp/process.h>
#include <commoncpp/file.h>

#ifdef _MSWINDOWS_
#include <sys/stat.h>
#include <malloc.h>
#endif

namespace ost {
using namespace std;

Dir::Dir(const char *fname) :
#ifdef _MSWINDOWS_
hDir(INVALID_HANDLE_VALUE), name(NULL)
#else
dir(NULL)
#endif
{
#ifdef HAVE_READDIR_R
       save = reinterpret_cast<struct dirent*>(save_space);
#endif
	if(fname)
		open(fname);
}

bool Dir::create(const char *path, Attr attr)
{
	bool rtn = true;
#ifdef	_MSWINDOWS_

	// fixme: make it form a security attributes structure

	if(!CreateDirectory(path, NULL))
		rtn = false;
#else
	long xmask = 0;

	switch(attr)
	{
	case attrPublic:
		xmask |= S_IXOTH;
	case attrGroup:
		xmask |= S_IXGRP;
	case attrPrivate:
		xmask |= S_IXUSR;
		break;
	default:
		return false;
	}
	if(mkdir(path, (long)attr | xmask))
		rtn = false;
#endif
	return rtn;
}

bool Dir::remove(const char *path)
{
	bool rtn = true;
#ifdef	_MSWINDOWS_
	if(!RemoveDirectory(path))
		rtn = false;
#else
	if(rmdir(path))
		rtn = false;
#endif
	return rtn;
}

bool Dir::setPrefix(const char *prefix)
{
	bool rtn = true;

#ifdef	_MSWINDOWS_
	if(!SetCurrentDirectory(prefix))
		rtn = false;
#else
	if(chdir(prefix))
		rtn = false;
#endif
	return rtn;
}

bool Dir::getPrefix(char *prefix, size_t size)
{
	bool rtn = true;

#ifdef	_MSWINDOWS_
	if(!GetCurrentDirectory((DWORD)size, prefix))
		rtn = false;
#else
	if(getcwd(prefix, size) == NULL)
		rtn = false;
#endif
	return rtn;
}

void Dir::open(const char *fname)
{
#ifdef	_MSWINDOWS_
	size_t len = strlen(fname) + 4;
	char *path;
#endif

	close();
#ifdef _MSWINDOWS_
	DWORD attr = GetFileAttributes(fname);
	if( (attr == (DWORD)~0l) || !(attr & FILE_ATTRIBUTE_DIRECTORY) )
	{
#ifdef	CCXX_EXCEPTIONS
		if(Thread::getException() == Thread::throwObject)
			throw(this);
#ifdef	COMMON_STD_EXCEPTION
		else if(Thread::getException() == Thread::throwException)
			throw(DirException(String(fname) + ": failed"));
#endif
#endif
	}

	path = (char *)_malloca(len + 1);
	if(path)
		snprintf(path, len + 1, "%s", fname);
#ifdef	CCXX_EXCEPTIONS
	if (!path && Thread::getException() == Thread::throwObject)
		throw(this);
#ifdef	COMMON_STD_EXCEPTION
	else if(!path && Thread::getException() == Thread::throwException)
		throw(DirException(String(fname) + ": failed"));
#endif
#endif
	if (!path || !path[0])
		return;
	addString(path, len, "\\*");
	hDir = FindFirstFile(path, &fdata);
	if(hDir != INVALID_HANDLE_VALUE)
		name = fdata.cFileName;
	memcpy(&data, &fdata, sizeof(fdata));

#else // WIN32
	entry = NULL;
	dir = opendir(fname);
#ifdef	CCXX_EXCEPTIONS
	if(!dir && Thread::getException() == Thread::throwObject)
		throw(this);
#ifdef	COMMON_STD_EXCEPTION
	else if(!dir && Thread::getException() == Thread::throwException)
		throw(DirException(String(fname) + ": failed"));
#endif
#endif
#endif // WIN32
}

Dir::~Dir()
{
	close();
}

void Dir::close(void)
{
#ifdef _MSWINDOWS_
	if(hDir != INVALID_HANDLE_VALUE)
		FindClose(hDir);
	hDir = INVALID_HANDLE_VALUE;
#else
	if(dir)
		closedir(dir);
	dir = NULL;
	entry = NULL;
#endif
}

bool Dir::rewind(void)
{
	bool rtn = true;
#ifdef	_MSWINDOWS_
	memcpy(&data, &fdata, sizeof(data));
	name = fdata.cFileName;
#else
	if(!dir)
		rtn = false;
	else
		rewinddir(dir);
#endif
	return rtn;
}

bool Dir::isValid(void) const
{
#ifdef _MSWINDOWS_
	if(hDir == INVALID_HANDLE_VALUE)
#else
	if(!dir)
#endif
		return false;

	return true;
}

const char *Dir::operator*()
{
#ifdef	_MSWINDOWS_
	return name;
#else
	if(!dir)
		return NULL;

	if(!entry)
		return getName();

	return entry->d_name;
#endif
}

const char *Dir::getName(void)
{
#ifdef _MSWINDOWS_
	char *retname = name;

	if(hDir == INVALID_HANDLE_VALUE)
		return NULL;

	if(retname)
	{
		name = NULL;
		if(FindNextFile(hDir, &data))
			name = data.cFileName;
	}
	return retname;

#else
	if(!dir)
		return NULL;

#ifdef	HAVE_READDIR_R
	readdir_r(dir, save, &entry);
#else
	entry = readdir(dir);
#endif
	if(!entry)
		return NULL;

	return entry->d_name;
#endif // WIN32
}

DirTree::DirTree(const char *prefix, unsigned depth)
{
	max = ++depth;
	dir = new Dir[depth];
	current = 0;

	open(prefix);
}

DirTree::DirTree(unsigned depth)
{
	max = ++depth;
	dir = new Dir[depth];
	current = 0;
}

void DirTree::open(const char *prefix)
{
	char *cp;

	close();

	if(!isDir(prefix))
		return;

	snprintf(path, sizeof(path), "%s/", prefix);
	prefixpos = (unsigned)strlen(path) - 1;

	while(NULL != (cp = strchr(path, '\\')))
		*cp = '/';

	while(prefixpos && path[prefixpos - 1] == '/')
		path[prefixpos--] = 0;

	dir[current++].open(prefix);
}

DirTree::~DirTree()
{
	close();

	if(dir)
		delete[] dir;

	dir = NULL;
}

unsigned DirTree::perform(const char *prefix)
{
	unsigned count = 0;

	open(prefix);
	while(NULL != (getPath()))
		++count;
	close();
	return count;
}

void DirTree::close(void)
{
	while(current--)
		dir[current].close();

	current = 0;
}


bool DirTree::filter(const char *fpath, struct stat *ino)
{
	fpath = strrchr(path, '/');

	if(fpath)
		++fpath;
	else
		return false;

	if(!strcmp(fpath, "."))
		return false;

	if(!strcmp(fpath, ".."))
		return false;

	if(!ino)
		return false;

	return true;
}

char *DirTree::getPath(void)
{
	char *cp;
	const char *name;
	struct stat ino;
	bool flag;

	while(current)
	{
		cp = strrchr(path, '/');
		name = dir[current - 1].getName();
		if(!name)
		{
			*cp = 0;
			dir[--current].close();
			continue;
		}
		snprintf(cp + 1, sizeof(path) - strlen(path) - 2, "%s", name);

		if(::stat(path, &ino))
		{
			ino.st_mode = 0;
			flag = filter(path, NULL);
		}
		else
			flag = filter(path, &ino);

		if(!flag)
			continue;

		if((ino.st_mode & S_IFMT) == S_IFDIR)
		{
			if(!canAccess(path))
				break;

			if(current < max)
				dir[current++].open(path);

			snprintf(path + strlen(path), sizeof(path) - strlen(path), "/");
		}
		break;
	}
	if(!current)
		return NULL;

	return path;
}

}	// end namespace
