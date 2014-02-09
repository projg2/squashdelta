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

#include "squashfs.hxx"
#include "util.hxx"

class Compressor
{
public:
	virtual ~Compressor();

	virtual void setup(MetadataReader* coptsr) = 0;
	virtual void reset();

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) = 0;

	virtual uint32_t get_compression_value() const = 0;
};

#ifdef ENABLE_LZO
class LZOCompressor : public Compressor
{
	int compression_level;
	bool optimized;
	bool optimized_tested;

public:
	LZOCompressor();

	virtual void setup(MetadataReader* coptsr);
	virtual void reset();

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size);

	virtual uint32_t get_compression_value() const;
};
#endif /*ENABLE_LZO*/

#ifdef ENABLE_LZ4
class LZ4Compressor : public Compressor
{
	bool hc;

public:
	LZ4Compressor();

	virtual void setup(MetadataReader* coptsr);

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size);

	virtual uint32_t get_compression_value() const;
};
#endif /*ENABLE_LZ4*/

#endif /*!SDT_COMPRESSOR_HXX*/
