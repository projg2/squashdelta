/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifndef SDT_UTIL_HXX
#define SDT_UTIL_HXX 1

#include <cstdlib> // size_t (maybe take it from somewhere else?)
#include <ios>
#include <stdexcept>
#include <string>

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

extern "C"
{
#include <sys/types.h>
#ifdef HAVE_STDINT_H
#	include <stdint.h>
#endif
#ifdef HAVE_ENDIAN_H
#	include <endian.h>
#endif
}

/**
 * Utility classes.
 */

// converters from LE to host
template <class T>
inline T le_to_host(T n);
template <>
inline uint16_t le_to_host(uint16_t n) { return le16toh(n); }
template <>
inline uint32_t le_to_host(uint32_t n) { return le32toh(n); }
template <>
inline uint64_t le_to_host(uint64_t n) { return le64toh(n); }

// magic auto-conversion types
template <class T>
class LittleEndian
{
	T data;

public:
	inline operator T() const
	{
		return le_to_host<T>(data);
	}
};

typedef LittleEndian<uint16_t> le16;
typedef LittleEndian<uint32_t> le32;
typedef LittleEndian<uint64_t> le64;

// few common exception types
class IOError : public std::runtime_error
{
public:
	int errno_val;

	IOError(const char* text, int new_errno);
};

// MMAP-based file reader
class MMAPFile
{
	int fd;
	size_t length;
	size_t pos;
	bool duplicate;

	void close();

public:
	MMAPFile();
	MMAPFile(const MMAPFile& ref);
	~MMAPFile();

	void open(const char* path);

	template <class T>
	void peek(T* out);
	template <class T>
	void read(T* out);

	template <class T>
	void peek_array(T* out, size_t n);
	template <class T>
	void read_array(T* out, size_t n);

	void read_bytes(uint8_t* out, size_t n);

	size_t getpos() const;
	size_t getlen() const;
	void seek(ssize_t offset,
			std::ios_base::seekdir whence = std::ios_base::cur);
};

template <class T>
void MMAPFile::peek(T* out)
{
	peek_array<T>(out, 1);
}

template <class T>
void MMAPFile::read(T* out)
{
	read_array<T>(out, 1);
}

template <class T>
void MMAPFile::peek_array(T* out, size_t n)
{
	read_array<T>(out, n);
	seek(-n * sizeof (T), std::ios::cur);
}

template <class T>
void MMAPFile::read_array(T* out, size_t n)
{
	read_bytes(static_cast<uint8_t*>(static_cast<void*>(out)),
			n * sizeof (T));
}

class SparseFileWriter
{
	off_t offset;

public:
	int fd;

	SparseFileWriter();
	virtual ~SparseFileWriter();

	void open(const char* path, off_t expected_size = 0);
	void close();

	void write(const void* data, size_t length);
	void write_sparse(size_t length);
	void copy_from(MMAPFile& f, size_t length);

	template <class T>
	void write(const T& data);
};

template <class T>
void SparseFileWriter::write(const T& data)
{
	write(&data, sizeof(T));
}

static const char tmpfile_template[] = "tmp.XXXXXX";

class TemporarySparseFileWriter : public SparseFileWriter
{
	char buf[sizeof(tmpfile_template)];
	pid_t parent_pid;

public:
	TemporarySparseFileWriter();
	virtual ~TemporarySparseFileWriter();

	void open(off_t expected_size = 0);
	void close();

	const char* name();
};

#endif /*!SDT_UTIL_HXX*/
