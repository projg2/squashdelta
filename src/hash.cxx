/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "hash.hxx"

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// modified by Michał Górny to be simpler to read and more fit to the code

static inline uint32_t rotl32(uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}

uint32_t murmurhash3(const void* key, size_t len, uint32_t seed)
{
//	const uint8_t* data = static_cast<const uint8_t*>(key);
	const uint32_t* blocks = static_cast<const uint32_t*>(key);

	const size_t nblocks = len / 4;

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	for(size_t i = 0; i < nblocks; ++i)
	{
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = rotl32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = rotl32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t* tail = static_cast<const uint8_t*>(
			static_cast<const void*>(blocks + nblocks));

	uint32_t k1 = 0;

	switch (len % 4)
	{
		// (pass-through intended)
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0];
			k1 *= c1;
			k1 = rotl32(k1,15);
			k1 *= c2;
			h1 ^= k1;
	};

	h1 ^= len;
	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;


	return h1;
}
