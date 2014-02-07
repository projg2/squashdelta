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

namespace compressor_id
{
	enum compressor_id
	{
		lzo = 0x01 << 24,
		mask = 0xff << 24
	};
}

Compressor::~Compressor()
{
}

#ifdef ENABLE_LZO

namespace lzo_options
{
	enum lzo_options
	{
		lzo1x_999 = 0x00,
		lzo1x_999_min = 0x01, // min compression level
		lzo1x_999_max = 0x09, // max compression level
		algo_level_mask = 0x0f,

		optimized = 0x10
	};
}

#pragma pack(push, 1)
namespace lzo
{
	struct comp_options
	{
		le32 algorithm;
		le32 compression_level;
	};

	namespace algorithm
	{
		enum algorithm
		{
			lzo1x_1 = 0,
			lzo1x_1_11 = 1,
			lzo1x_1_12 = 2,
			lzo1x_1_15 = 3,
			lzo1x_999 = 4
		};
	}
}
#pragma pack(pop)

LZOCompressor::LZOCompressor(const void* comp_options,
		size_t comp_opt_length)
	: compression_level(8) // default
{
	if (comp_options)
	{
		const struct lzo::comp_options& opts
			= *static_cast<const struct lzo::comp_options*>(comp_options);

		if (comp_opt_length < sizeof(opts))
			throw std::runtime_error("Compression options too short");

		if (opts.algorithm != lzo::algorithm::lzo1x_999)
			throw std::runtime_error("Only lzo1x_999 algorithm is supported");

		if (opts.compression_level < 1 || opts.compression_level > 9)
			throw std::runtime_error("Invalid compression level specified");

		compression_level = opts.compression_level;
	}

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

uint32_t LZOCompressor::get_compression_value() const
{
	// default algo: lzo1x_999
	// default level: 8
	// optimized since 4.3

	return compressor_id::lzo
		| lzo_options::lzo1x_999
		| compression_level;
//		| lzo_options::optimized;
}

#endif /*ENABLE_LZO*/
