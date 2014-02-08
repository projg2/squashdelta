/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once
#ifndef SDT_COMPRESSOR_HXX
#define SDT_COMPRESSOR_HXX 1

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cstdlib>

extern "C"
{
#ifdef HAVE_STDINT_H
#	include <stdint.h>
#endif
}

#include "util.hxx"

class Compressor
{
public:
	virtual ~Compressor();

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) const = 0;

	virtual uint32_t get_compression_value() const = 0;
};

#ifdef ENABLE_LZO
class LZOCompressor : public Compressor
{
	int compression_level;

public:
	LZOCompressor(const void* comp_options, size_t comp_opt_length);

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) const;

	virtual uint32_t get_compression_value() const;
};
#endif /*ENABLE_LZO*/

#ifdef ENABLE_LZ4
class LZ4Compressor : public Compressor
{
	bool hc;

public:
	LZ4Compressor(const void* comp_options, size_t comp_opt_length);

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) const;

	virtual uint32_t get_compression_value() const;
};
#endif /*ENABLE_LZ4*/

#endif /*!SDT_COMPRESSOR_HXX*/
