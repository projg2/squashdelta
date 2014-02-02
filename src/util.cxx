/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cerrno>

extern "C"
{
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <sys/mman.h>
#	include <fcntl.h>
#	include <unistd.h>
}

#include "util.hxx"

IOError::IOError(const char* text, int new_errno)
	: std::runtime_error(text),
	errno_val(new_errno)
{
}

MMAPFile::MMAPFile()
	: fd(-1), pos(0), data(0)
{
}


MMAPFile::MMAPFile(const MMAPFile& ref)
	// just copy the data necessary for read/seek
	// but not the one needed to close/unmap
	: fd(-1), pos(ref.pos), end(ref.end), data(0)
{
}

MMAPFile::~MMAPFile()
{
	close();
}

void MMAPFile::open(const char* path)
{
	fd = ::open(path, O_RDONLY);
	if (fd == -1)
		throw IOError("Unable to open file", errno);

	// this also checks whether the file is seekable
	off_t size = lseek(fd, 0, SEEK_END);
	if (size == -1)
	{
		close();
		throw IOError("Unable to seek file (not a regular file?)", errno);
	}

	// size_t <- off_t
	length = size;

	data = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
	{
		close();
		throw IOError("Unable to mmap() file", errno);
	}

	pos = static_cast<char*>(data);
	end = pos + length;
}

void MMAPFile::close()
{
	bool munmap_failed = false;
	bool close_failed = false;

	if (data)
	{
		if (::munmap(data, length) == -1)
			munmap_failed = true;
		data = 0;
		pos = 0;
	}

	if (fd != -1)
	{
		if (::close(fd) == -1)
			close_failed = true;
		fd = -1;
	}

	if (munmap_failed && close_failed)
		throw IOError("Unable to unmap and close file", errno);
	else if (munmap_failed)
		throw IOError("Unable to unmap file (yet it was closed)", errno);
	else if (close_failed)
		throw IOError("Unable to close file", errno);
}

void MMAPFile::seek(size_t offset)
{
	if (!pos || pos + offset > end)
		throw std::runtime_error("EOF while seeking");

	pos += offset;
}
