/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once
#ifndef SDT_COMPRESSOR_HXX
#define SDT_COMPRESSOR_HXX 1

#include <cstdlib>

class Compressor
{
public:
	virtual ~Compressor();

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) const = 0;
};

class LZOCompressor : public Compressor
{
public:
	LZOCompressor();

	virtual size_t decompress(void* dest, const void* src,
			size_t length, size_t out_size) const;
};

#endif /*!SDT_COMPRESSOR_HXX*/
