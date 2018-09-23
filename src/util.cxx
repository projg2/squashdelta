/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cerrno>
#include <cstring>

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
	: fd(-1), pos(0), length(0), data(0)
{
}


MMAPFile::MMAPFile(const MMAPFile& ref)
	// just copy the data necessary for read/seek
	// but not the one needed to close/unmap
	: fd(-1), pos(ref.pos), end(ref.end), length(0), data(ref.data)
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

	if (data && length > 0)
	{
		if (::munmap(data, length) == -1)
			munmap_failed = true;
		data = 0;
		length = 0;
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

size_t MMAPFile::getpos() const
{
	if (!data)
		throw std::logic_error("getpos() for closed file");

	return (pos - static_cast<char*>(data));
}

void MMAPFile::seek(ssize_t offset, std::ios_base::seekdir whence)
{
	char* newpos;

	if (!data)
		throw std::logic_error("Seeking closed file");

	switch (whence)
	{
		case std::ios::beg:
			newpos = static_cast<char*>(data);
			break;
		case std::ios::cur:
			newpos = pos;
			break;
		case std::ios::end:
			newpos = end;
			break;
		default:
			throw std::logic_error("Invalid value for whence");
	}

	newpos += offset;
	if (newpos > end)
		throw std::runtime_error("EOF while seeking");

	pos = newpos;
}

SparseFileWriter::SparseFileWriter()
	: offset(0), fd(-1)
{
}

SparseFileWriter::~SparseFileWriter()
{
	if (fd != -1)
		::close(fd);
}

void SparseFileWriter::open(const char* path, off_t expected_size)
{
	fd = creat(path, 0666);
	if (fd == -1)
		throw IOError("Unable to create file", errno);

	if (expected_size > 0)
		posix_fallocate(fd, 0, expected_size);
}

void SparseFileWriter::close()
{
	if (fd == -1)
		throw std::runtime_error("File is already closed!");

	if (::close(fd) == -1)
		throw IOError("close() failed", errno);
	fd = -1;
}

void SparseFileWriter::write(const void* data, size_t length)
{
	const char* buf = static_cast<const char*>(data);

	while (length > 0)
	{
		ssize_t ret = ::write(fd, buf, length);

		if (ret == -1)
			throw IOError("write() failed", errno);
		length -= ret;
		buf += ret;
		offset += ret;
	}
}

void SparseFileWriter::write_sparse(size_t length)
{
	off_t past = offset + length;

	if (ftruncate(fd, past) == -1)
		throw IOError("ftruncate() failed to extend the sparse file", errno);
	if (lseek(fd, length, SEEK_CUR) == -1)
		throw IOError("lseek() failed to seek past sparse block", errno);

	offset = past;
}

TemporarySparseFileWriter::TemporarySparseFileWriter()
{
}

TemporarySparseFileWriter::~TemporarySparseFileWriter()
{
	if (buf[0] == '\0')
		return;

	// unlink the file only in parent process
	if (parent_pid == getpid())
		unlink(name());
}

void TemporarySparseFileWriter::open(off_t expected_size)
{
	parent_pid = getpid();

	strcpy(buf, tmpfile_template);
	fd = mkstemp(buf);
	if (fd == -1)
		throw IOError("Unable to create a temporary file", errno);

	if (expected_size > 0)
		posix_fallocate(fd, 0, expected_size);
}

const char* TemporarySparseFileWriter::name()
{
	return buf;
}

void TemporarySparseFileWriter::close()
{
	SparseFileWriter::close();

	// unlink the file only in parent process
	if (parent_pid == getpid() && unlink(name()) == -1)
		throw IOError("Unable to unlink the temporary file", errno);
	buf[0] = '\0';
}
