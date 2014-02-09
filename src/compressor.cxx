/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cstring>
#include <stdexcept>

#ifdef ENABLE_LZO
#	include <lzo/lzo1x.h>
#endif
#ifdef ENABLE_LZ4
#	include <lz4.h>
#	include <lz4hc.h>
#endif

#include "compressor.hxx"

namespace compressor_id
{
	enum compressor_id
	{
		lzo = 0x01 << 24,
		lz4 = 0x02 << 24,
		mask = 0xff << 24
	};
}

Compressor::~Compressor()
{
}

void Compressor::reset()
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

LZOCompressor::LZOCompressor()
	: compression_level(8), optimized(true), // default
	optimized_tested(false)
{
	if (lzo_init() != LZO_E_OK)
		throw std::runtime_error("lzo_init() failed");
}

void LZOCompressor::setup(const void* comp_options, size_t comp_opt_length)
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
}

void LZOCompressor::reset()
{
	optimized_tested = false;
}

size_t LZOCompressor::decompress(void* dest, const void* src,
		size_t length, size_t out_size)
{
	const unsigned char* src2 = static_cast<const unsigned char*>(src);
	unsigned char* dest2 = static_cast<unsigned char*>(dest);

	lzo_uint out_bytes = out_size;

	if (lzo1x_decompress_safe(src2, length, dest2, &out_bytes, 0) != LZO_E_OK)
		throw std::runtime_error("LZO decompression failed (corrupted data?)");

	// check whether the output was optimized
	if (!optimized_tested)
	{
		int ret, ret2;

		char workspace[LZO1X_999_MEM_COMPRESS];
		unsigned char* cbuf = new unsigned char[length];
		lzo_uint comp_bytes = length;

		ret = lzo1x_999_compress_level(dest2, out_bytes, cbuf, &comp_bytes,
					workspace, 0, 0, 0, compression_level) != LZO_E_OK;

		if (ret == LZO_E_OK && comp_bytes == length)
		{
			unsigned char* obuf = new unsigned char[length];
			unsigned char* ibuf = new unsigned char[out_bytes];

			lzo_uint out_bytes2 = out_bytes;

			memcpy(obuf, cbuf, length);
			ret2 = lzo1x_optimize(obuf, length, ibuf, &out_bytes2, 0);

			// first of all, check whether optimization changes anything
			// if it does not, we need to try on another block
			if (memcmp(obuf, cbuf, length))
			{
				// we assume the output was optimized if we get the same
				// result after re-compressing and optimizing
				optimized = !memcmp(src, obuf, length);

				optimized_tested = true;
			}

			delete[] ibuf;
			delete[] obuf;
		}

		// if it was not optimized, we should get the same result
		// as for plain compression. otherwise, raise an exception
		if (!optimized && memcmp(src, cbuf, length))
		{
			delete[] cbuf;

			throw std::runtime_error("Input compressed data does not match"
					" re-compressed optimized nor non-optimized data");
		}

		delete[] cbuf;

		if (ret != LZO_E_OK)
			throw std::runtime_error("LZO test re-compression failed");
		if (comp_bytes != length)
			throw std::runtime_error("LZO test re-compression resulted in different size");
		if (ret2 != LZO_E_OK)
			throw std::runtime_error("LZO test re-optimization failed");
	}

	return out_bytes;
}

uint32_t LZOCompressor::get_compression_value() const
{
	// default algo: lzo1x_999
	// default level: 8
	// optimized since 4.3

	uint32_t ret = compressor_id::lzo
		| lzo_options::lzo1x_999
		| compression_level;

	if (optimized)
		ret |= lzo_options::optimized;

	return ret;
}

#endif /*ENABLE_LZO*/

#ifdef ENABLE_LZ4

namespace lz4_options
{
	enum lz4_options
	{
		hc = 1
	};
}

#pragma pack(push, 1)
namespace lz4
{
	struct comp_options
	{
		le32 version;
		le32 flags;
	};

	namespace version
	{
		enum version
		{
			legacy = 1
		};
	}

	namespace flags
	{
		enum flags
		{
			hc = 1,

			flags_mask = hc
		};
	}
}
#pragma pack(pop)

LZ4Compressor::LZ4Compressor()
{
}

void LZ4Compressor::setup(const void* comp_options, size_t comp_opt_length)
{
	if (comp_options)
	{
		const struct lz4::comp_options& opts
			= *static_cast<const struct lz4::comp_options*>(comp_options);

		if (comp_opt_length < sizeof(opts))
			throw std::runtime_error("Compression options too short");

		if (opts.version != lz4::version::legacy)
			throw std::runtime_error("Unsupported LZ4 stream version");

		if ((opts.flags & ~lz4::flags::flags_mask) != 0)
			throw std::runtime_error("Unknown LZ4 flags found");

		hc = opts.flags & lz4::flags::hc;
	}
	else
		throw std::runtime_error("No compression options for LZ4 found");
}

size_t LZ4Compressor::decompress(void* dest, const void* src,
		size_t length, size_t out_size)
{
	const char* src2 = static_cast<const char*>(src);
	char* dest2 = static_cast<char*>(dest);
	int out;

	out = LZ4_decompress_safe(src2, dest2, length, out_size);

	if (out < 0)
		throw std::runtime_error("LZ4 decompression failed (corrupted data?)");

	return out;
}

uint32_t LZ4Compressor::get_compression_value() const
{
	uint32_t ret = compressor_id::lz4;

	if (hc)
		ret |= lz4_options::hc;

	return ret;
}

#endif /*ENABLE_LZ4*/
