/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifndef SDT_UTIL_HXX
#define SDT_UTIL_HXX 1

#include <cstdlib> // size_t (maybe take it from somewhere else?)
#include <stdexcept>
#include <string>

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

extern "C"
{
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
	char* pos;
	char* end;

	void close();

public:
	size_t length;
	void* data;

	MMAPFile();
	MMAPFile(const MMAPFile& ref);
	~MMAPFile();

	void open(const char* path);

	template <class T>
	const T& peek();
	template <class T>
	const T& read();

	template <class T>
	const T* peek_array(size_t n);
	template <class T>
	const T* read_array(size_t n);

	void seek(size_t offset);
};

template <class T>
const T& MMAPFile::peek()
{
	return *peek_array<T>(1);
}

template <class T>
const T& MMAPFile::read()
{
	return *read_array<T>(1);
}

template <class T>
const T* MMAPFile::peek_array(size_t n)
{
	// ensure we don't run out of data :)
	if (!pos || pos + sizeof(T) * n > end)
		throw std::runtime_error("EOF while reading");

	return static_cast<T*>(static_cast<void*>(pos));
}

template <class T>
const T* MMAPFile::read_array(size_t n)
{
	const T* ret = peek_array<T>(n);
	seek(sizeof(T) * n);
	return ret;
}

#endif /*!SDT_UTIL_HXX*/
