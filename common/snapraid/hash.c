/*
 * Copyright (C) 2011 Andrea Mazzoleni
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util.h"
#include "cpu.h"

/****************************************************************************/
/* hash */

/* Rotate left 32 */
inline uint32_t rotl32(uint32_t x, int8_t r)
{
	return (x << r) | (x >> (32 - r));
}

inline uint64_t rotl64(uint64_t x, int8_t r)
{
	return (x << r) | (x >> (64 - r));
}

/* Swap endianess */
#if HAVE_BYTESWAP_H

#include <byteswap.h>

#define swap32(x) bswap_32(x)
#define swap64(x) bswap_64(x)

#else
static inline uint32_t swap32(uint32_t v)
{
	return (rotl32(v, 8) & 0x00ff00ff)
		| (rotl32(v, 24) & 0xff00ff00);
}

static inline uint64_t swap64(uint64_t v)
{
	return (rotl64(v, 8) & 0x000000ff000000ffLLU)
		| (rotl64(v, 24) & 0x0000ff000000ff00LLU)
		| (rotl64(v, 40) & 0x00ff000000ff0000LLU)
		| (rotl64(v, 56) & 0xff000000ff000000LLU);
}
#endif

#include "murmur3.c"
#include "spooky2.c"

void memhash(unsigned kind, const unsigned char* seed, void* digest, const void* src, unsigned size)
{
	switch (kind) {
	case HASH_MURMUR3 :
		MurmurHash3_x86_128(src, size, seed, digest);
		break;
	case HASH_SPOOKY2 :
		SpookyHash128(src, size, seed, digest);
		break;
	default:
		fprintf(stderr, "Internal inconsistency in hash function %u\n", kind);
		exit(EXIT_FAILURE);
		break;
	}
}

const char* hash_config_name(unsigned kind)
{
	switch (kind) {
	case HASH_UNDEFINED : return "undefined";
	case HASH_MURMUR3 : return "murmur3";
	case HASH_SPOOKY2 : return "spooky2";
	default: return "unknown";
	}
}

/****************************************************************************/
/* random */

void randomize(void* void_ptr, unsigned size)
{
	unsigned char* ptr = void_ptr;
	unsigned i;

	srand(time(0));

	for(i=0;i<size;++i)
		ptr[i] = rand();
}

