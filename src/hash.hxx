/**
 * SquashFS delta tools
 * (c) 2014 Michał Górny
 * Released under the terms of the 2-clause BSD license
 */

#pragma once

#ifndef SDT_HASH_HXX
#define SDT_HASH_HXX 1

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

uint32_t murmurhash3(const void* key, size_t len, uint32_t seed);

#endif /*!SDT_HASH_HXX*/
