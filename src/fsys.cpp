// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/fsys.h>
#include <ucommon/string.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef	HAVE_DIRENT_H
#include <dirent.h>
#endif

using namespace UCOMMON_NAMESPACE;

const size_t fsys::end = (size_t)(-1);

#ifdef	_MSWINDOWS_

int remapError(void)
{
	DWORD err = GetLastError();

	switch(err)
	{
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_NAME:
	case ERROR_BAD_PATHNAME:
		return ENOENT;
	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;
	case ERROR_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return ENOMEM;
	case ERROR_INVALID_DRIVE:
	case ERROR_BAD_UNIT:
	case ERROR_BAD_DEVICE:
		return ENODEV;
	case ERROR_NOT_SAME_DEVICE:
		return EXDEV;
	case ERROR_NOT_SUPPORTED:
	case ERROR_CALL_NOT_IMPLEMENTED:
		return ENOSYS;
	case ERROR_END_OF_MEDIA:
	case ERROR_EOM_OVERFLOW:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_DISK_FULL:
		return ENOSPC;
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_NET_NAME:
		return EACCES;
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_CANNOT_MAKE:
	case ERROR_NOT_OWNER:
		return EPERM;
	case ERROR_NO_PROC_SLOTS:
		return EAGAIN;
	case ERROR_BROKEN_PIPE:
	case ERROR_NO_DATA:
		return EPIPE;
	case ERROR_OPEN_FAILED:
		return EIO;
	case ERROR_NOACCESS:
		return EFAULT;
	case ERROR_IO_DEVICE:
	case ERROR_CRC:
	case ERROR_NO_SIGNAL_SENT:
		return EIO;
	case ERROR_CHILD_NOT_COMPLETE:
	case ERROR_SIGNAL_PENDING:
	case ERROR_BUSY:
		return EBUSY;
	default:
		return EINVAL;
	}
}

int fsys::removeDir(const char *path)
{
	return _rmdir(path);
}

int fsys::createDir(const char *path, unsigned mode)
{
	if(!CreateDirectory(path, NULL))
		return -1;
	
	return _chmod(path, mode);
}

int fsys::stat(const char *path, struct stat *buf)
{
	return _stat(path, (struct _stat *)(buf));
}

int fsys::stat(struct stat *buf)
{
	int fn = _open_osfhandle((long int)(fd), _O_RDONLY);
	
	int rtn = _fstat(fn, (struct _stat *)(buf));
	_close(fn);
	if(rtn)
		error = errno;
	return rtn;
}	 

int fsys::setPrefix(const char *path)
{
	return _chdir(path);
}

char *fsys::getPrefix(char *path, size_t len)
{
	return _getcwd(path, len);
}

int fsys::change(const char *path, unsigned mode)
{
	return _chmod(path, mode);
}

int fsys::access(const char *path, unsigned mode)
{
	return _access(path, mode);
}

int fsys::close(fd_t fd)
{
	if(CloseHandle(fd))
		return 0;
	errno = remapError();
	return -1;
}

void fsys::close(fsys &fs)
{
	if(fs.fd == INVALID_HANDLE_VALUE)
		return;

	if(CloseHandle(fs.fd)) {
		fs.fd = INVALID_HANDLE_VALUE;
		fs.error = 0;
	}
	else
		fs.error = errno = remapError();
}

ssize_t fsys::read(fd_t fd, void *buf, size_t len)
{
	DWORD count;
	if(ReadFile(fd, (LPVOID) buf, (DWORD)len, &count, NULL))
		return (ssize_t)(count);
	
	errno = remapError();
	return -1;
}

ssize_t fsys::read(void *buf, size_t len)
{
	DWORD count;
	if(ReadFile(fd, (LPVOID) buf, (DWORD)len, &count, NULL))
		return (ssize_t)(count);
	
	error = errno = remapError();
	return -1;
}

ssize_t fsys::write(fd_t fd, const void *buf, size_t len)
{
	DWORD count;
	if(WriteFile(fd, (LPCVOID) buf, (DWORD)len, &count, NULL))
		return (ssize_t)(count);
	
	errno = remapError();
	return -1;
}

ssize_t fsys::write(const void *buf, size_t len)
{
	DWORD count;
	if(WriteFile(fd, (LPCVOID) buf, (DWORD)len, &count, NULL))
		return (ssize_t)(count);
	
	error = errno = remapError();
	return -1;
}

fd_t fsys::open(const char *path, access_t access, unsigned mode)
{
	bool append = false;
	DWORD amode;
	DWORD cmode;
	DWORD attr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		amode = GENERIC_READ;
		cmode = OPEN_EXISTING;
		break;
	case ACCESS_WRONLY:
		amode = GENERIC_WRITE;
		cmode = CREATE_ALWAYS; 
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		break;
	case ACCESS_CREATE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = CREATE_NEW;		
		break;
	case ACCESS_APPEND:
		amode = GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		append = true;
		break;
	}
	HANDLE fd = CreateFile(path, amode, 0, NULL, cmode, attr, NULL);
	if(fd != INVALID_HANDLE_VALUE && append)
		setPosition(fd, end);
	else
		errno = remapError();
	if(fd != INVALID_HANDLE_VALUE)
		change(path, mode);
	return fd;	
}

void fsys::open(fsys &fs, const char *path, access_t access, unsigned mode)
{
	bool append = false;
	DWORD amode;
	DWORD cmode;
	DWORD attr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		amode = GENERIC_READ;
		cmode = OPEN_EXISTING;
		break;
	case ACCESS_WRONLY:
		amode = GENERIC_WRITE;
		cmode = CREATE_ALWAYS; 
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		break;
	case ACCESS_CREATE:
		amode = GENERIC_READ | GENERIC_WRITE;
		cmode = CREATE_NEW;		
		break;
	case ACCESS_APPEND:
		amode = GENERIC_WRITE;
		cmode = OPEN_ALWAYS;
		append = true;
		break;
	}

	close(fs);
	if(fs.fd != INVALID_HANDLE_VALUE)
		return;

	fs.fd = CreateFile(path, amode, 0, NULL, cmode, attr, NULL);
	if(fs.fd != INVALID_HANDLE_VALUE && append)
		fs.setPosition(end);
	else
		errno = fs.error = remapError();
	if(fs.fd != INVALID_HANDLE_VALUE)
		change(path, mode);
}

fsys::fsys(const fsys& copy)
{
	HANDLE pHandle = GetCurrentProcess();
	if(DuplicateHandle(pHandle, copy.fd, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		error = 0;
	else {
		fd = INVALID_HANDLE_VALUE;
		error = errno = remapError();
	}
}

void fsys::operator=(fd_t from)
{
	HANDLE pHandle = GetCurrentProcess();

	if(fd != INVALID_HANDLE_VALUE) {
		if(!CloseHandle(fd)) {
			errno = error = remapError();
			return;
		}
	}
	if(DuplicateHandle(pHandle, from, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		error = 0;
	else {
		fd = INVALID_HANDLE_VALUE;
		error = errno = remapError();
	}
}

void fsys::operator=(fsys &from)
{
	HANDLE pHandle = GetCurrentProcess();

	if(fd != INVALID_HANDLE_VALUE) {
		if(!CloseHandle(fd)) {
			errno = error = remapError();
			return;
		}
	}
	if(DuplicateHandle(pHandle, from.fd, pHandle, &fd, 0, FALSE, DUPLICATE_SAME_ACCESS))
		error = 0;
	else {
		fd = INVALID_HANDLE_VALUE;
		error = errno = remapError();
	}
}

void fsys::setPosition(size_t pos)
{
	DWORD rpos = pos;
	int mode = FILE_BEGIN;

	if(rpos == end) {
		rpos = 0;
		mode = FILE_END;
	}
	if(SetFilePointer(fd, rpos, NULL, mode) == INVALID_SET_FILE_POINTER)
		error = errno = remapError();
}

int fsys::setPosition(fd_t fd, size_t pos)
{
	DWORD rpos = pos;
	int mode = FILE_BEGIN;

	if(rpos == end) {
		rpos = 0;
		mode = FILE_END;
	}
	if(SetFilePointer(fd, rpos, NULL, mode) == INVALID_SET_FILE_POINTER) {
		errno = remapError();
		return -1;
	}
	return 0;
}

#else
#ifdef	__PTH__

ssize_t fsys::read(fd_t fd, void *buf, size_t len)
{
	return pth_read(fd, buf, len);
}

ssize_t fsys::read(void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = pth_read(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

ssize_t fsys::write(fd_t fd, const void *buf, size_t len)
{
	return pth_write(fd, buf, len);
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = pth_write(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

#else

ssize_t fsys::read(fd_t fd, void *buf, size_t len)
{
	return ::read(fd, buf, len);
}

ssize_t fsys::read(void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = ::read(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

ssize_t fsys::write(fd_t fd, const void *buf, size_t len)
{
	return ::write(fd, buf, len);
}

ssize_t fsys::write(const void *buf, size_t len)
{
	if(fd == INVALID_HANDLE_VALUE) {
		error = EBADF;
		return -1;
	}

	int rtn = ::write(fd, buf, len);
	if(rtn <= 0)
		error = errno;
	return rtn;
}

#endif

int fsys::close(fd_t fd)
{
	return ::close(fd);
}

void fsys::close(fsys &fs)
{
	if(fs.fd == INVALID_HANDLE_VALUE)
		return;

	if(::close(fs.fd) == 0) {
		fs.fd = INVALID_HANDLE_VALUE;
		fs.error = 0;
	}
	else
		fs.error = errno;
}

fd_t fsys::open(const char *path, access_t access, unsigned mode)
{
	unsigned flags = 0;
	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		flags = O_RDWR | O_CREAT;
		break;
	case ACCESS_CREATE:
		flags = O_RDWR | O_TRUNC | O_CREAT | O_EXCL;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_CREAT | O_APPEND;
		break;
	}
	return ::open(path, flags, mode);
}

void fsys::open(fsys &fs, const char *path, access_t access, unsigned mode)
{
	unsigned flags = 0;

	switch(access)
	{
	case ACCESS_RDONLY:
		flags = O_RDONLY;
		break;
	case ACCESS_WRONLY:
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		break;
	case ACCESS_REWRITE:
		flags = O_RDWR | O_CREAT;
		break;
	case ACCESS_CREATE:
		flags = O_RDWR | O_TRUNC | O_CREAT | O_EXCL;
		break;
	case ACCESS_APPEND:
		flags = O_RDWR | O_CREAT | O_APPEND;
		break;
	}
	close(fs);
	if(fs.fd != INVALID_HANDLE_VALUE)
		return;

	fs.fd = ::open(path, flags, mode);
	if(fs.fd == INVALID_HANDLE_VALUE)
		fs.error = errno;
}

int fsys::stat(const char *path, struct stat *ino)
{
	return ::stat(path, ino);
}

int fsys::stat(struct stat *ino)
{
	int rtn = ::fstat(fd, ino);
	if(rtn)
		error = errno;
	return rtn;
}

int fsys::createDir(const char *path, unsigned mode)
{
	return ::mkdir(path, mode);
}

int fsys::removeDir(const char *path)
{
	return ::rmdir(path);
}

int fsys::setPrefix(const char *path)
{
	return ::chdir(path);
}

char *fsys::getPrefix(char *path, size_t len)
{
	return ::getcwd(path, len);
}

int fsys::change(const char *path, unsigned mode)
{
	return ::chmod(path, mode);
}

int fsys::access(const char *path, unsigned mode)
{
	return ::access(path, mode);
}

fsys::fsys(const fsys& copy)
{
	fd = dup(copy.fd);
	error = 0;
}

void fsys::operator=(fd_t from)
{
	if(fsys::close(fd) == 0) {
		fd = dup(from);
		error = 0;
	}
	else
		error = errno;
}

void fsys::operator=(fsys &from)
{
	if(fsys::close(fd) == 0) {
		fd = dup(from.fd);
		error = 0;
	}
	else
		error = errno;
}

void fsys::setPosition(size_t pos)
{
	unsigned long rpos = pos;
	int mode = SEEK_SET;

	if(rpos == end) {
		rpos = 0;
		mode = SEEK_END;
	}
	if(lseek(fd, mode, rpos))
		error = errno;
}

int fsys::setPosition(fd_t fd, size_t pos)
{
	unsigned long rpos = pos;
	int mode = SEEK_SET;

	if(rpos == end) {
		rpos = 0;
		mode = SEEK_END;
	}
	return lseek(fd, mode, rpos);
}

#endif

fsys::fsys()
{
	fd = INVALID_HANDLE_VALUE;
	error = 0;
}

fsys::fsys(const char *path, access_t access, unsigned mode)
{
	fd = fsys::open(path, access, mode);
}

fsys::~fsys()
{
	if(fsys::close(fd) == 0) {
		fd = INVALID_HANDLE_VALUE;
		error = 0;
	}
	else
		error = errno;
}

int fsys::remove(const char *path)
{
	return ::remove(path);
}

int fsys::rename(const char *oldpath, const char *newpath)
{
	return ::rename(oldpath, newpath);
}

#if	defined(_MSWINDOWS_)

typedef struct
{
	HANDLE dir;
	WIN32_FIND_DATA entry;
}	dnode_t;

dir_t fsys::openDir(const char *path)
{
	char tpath[256];
	DWORD attr = GetFileAttributes(path);
	dnode_t *node;

	if((attr == (DWORD)~0l) || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		return NULL;
	
	snprintf(tpath, sizeof(tpath), "%s%s", path, "\\*");
	node = new dnode_t;
	node->dir = FindFirstFile(tpath, &node->entry);
	if(node->dir == INVALID_HANDLE_VALUE) {
		delete node;
		return NULL;
	}
	return node;
}	

void fsys::closeDir(dir_t d)
{
	dnode_t *node = (dnode_t *)d;

	if(!node)
		return;

	if(node->dir != INVALID_HANDLE_VALUE)
		FindClose(node->dir);
	
	delete node;
}

char *fsys::readDir(dir_t d, char *buf, size_t len)
{
	dnode_t *node = (dnode_t *)d;
	*buf = 0;

	if(!node || node->dir == INVALID_HANDLE_VALUE)
		return NULL;

	snprintf(buf, len, node->entry.cFileName);
	if(!FindNextFile(node->dir, &node->entry)) {
		FindClose(node->dir);
		node->dir = INVALID_HANDLE_VALUE;
	}
	return buf;
}

#elif defined(HAVE_DIRENT_H)

dir_t fsys::openDir(const char *path)
{
	return (dir_t)::opendir(path);
}

void fsys::closeDir(dir_t dir)
{
	::closedir((DIR *)dir);
}

char *fsys::readDir(dir_t dir, char *buf, size_t len)
{
	dirent *entry = ::readdir((DIR *)dir);	
	
	*buf = 0;
	if(!entry)
		return NULL;
	
	String::set(buf, len, entry->d_name);
	return buf;
}

#endif
