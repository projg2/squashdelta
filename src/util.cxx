/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cassert>
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
	: fd(-1), pos(0), length(0), duplicate(false)
{
}

MMAPFile::MMAPFile(const MMAPFile& ref)
	: fd(ref.fd), pos(ref.pos), length(ref.length), duplicate(true)
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
	off_t ret = lseek(fd, 0, SEEK_END);
	if (ret == -1)
	{
		close();
		throw IOError("Unable to seek file (not a regular file?)", errno);
	}

	// size_t <- off_t
	length = ret;

	ret = lseek(fd, 0, SEEK_SET);
	if (ret == -1)
	{
		close();
		throw IOError("Unable to seek file back (not a regular file?)", errno);
	}

	pos = 0;
}

void MMAPFile::close()
{
	if (fd != -1 && !duplicate)
	{
		int fd_copy = fd;
		fd = -1;

		if (::close(fd_copy) == -1)
			throw IOError("Unable to close file", errno);
	}
}

void MMAPFile::read_bytes(uint8_t* out, size_t n)
{
	// reset the position
	// TODO: render this unnecessary by stopping to use multiple file
	// instances
	ssize_t ret = lseek(fd, pos, SEEK_SET);
	if (ret == -1)
		throw IOError("Unable to seek file before reading", errno);

	while (n > 0)
	{
		ssize_t rd = ::read(fd, out, n);
		if (rd == -1)
		{
			if (errno != EAGAIN)
				throw IOError("Unable to read file", errno);
		}
		else if (rd == 0) // EOF
			throw std::runtime_error("EOF while reading");
		else
		{
			out += rd;
			pos += rd;
			n -= rd;
		}
	}
}

size_t MMAPFile::getpos() const
{
	if (fd == -1)
		throw std::logic_error("getpos() for closed file");

	assert(lseek(fd, 0, SEEK_CUR) == pos);
	return pos;
}

size_t MMAPFile::getlen() const
{
	if (fd == -1)
		throw std::logic_error("getlen() for closed file");

	return length;
}

void MMAPFile::seek(ssize_t offset, std::ios_base::seekdir whence)
{
	int new_whence;

	if (fd == -1)
		throw std::logic_error("Seeking closed file");

	switch (whence)
	{
		case std::ios::beg:
			new_whence = SEEK_SET;
			break;
		case std::ios::cur:
			new_whence = SEEK_CUR;
			break;
		case std::ios::end:
			new_whence = SEEK_END;
			break;
		default:
			throw std::logic_error("Invalid value for whence");
	}

	ssize_t ret = lseek(fd, offset, new_whence);
	if (ret == -1)
		throw IOError("Unable to seek file", errno);

	pos = ret;
	if (pos > length)
		throw std::runtime_error("EOF while seeking");
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

void SparseFileWriter::copy_from(MMAPFile& f, size_t length)
{
	uint8_t buf[BUFSIZ];

	while (length > 0)
	{
		size_t sz = length > BUFSIZ ? BUFSIZ : length;
		f.read_array(buf, sz);
		write(buf, sz);
		length -= sz;
	}
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
