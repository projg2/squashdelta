/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdexcept>

#ifdef ENABLE_LZO
#	include <lzo/lzo1x.h>
#endif

#include "compressor.hxx"

Compressor::~Compressor()
{
}

#ifdef ENABLE_LZO

LZOCompressor::LZOCompressor()
{
	if (lzo_init() != LZO_E_OK)
		throw std::runtime_error("lzo_init() failed");
}

size_t LZOCompressor::decompress(void* dest, const void* src,
		size_t length, size_t out_size) const
{
	const unsigned char* src2 = static_cast<const unsigned char*>(src);
	unsigned char* dest2 = static_cast<unsigned char*>(dest);

	lzo_uint out_bytes = out_size;

	if (lzo1x_decompress_safe(src2, length, dest2, &out_bytes, 0) != LZO_E_OK)
		throw std::runtime_error("LZO decompression failed (corrupted data?)");

	return out_bytes;
}

#endif /*ENABLE_LZO*/
