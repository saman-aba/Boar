#ifndef __HASH_H__
#define __HASH_H__

#include <stdint.h>

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

#define hash_long(val, bits) hash_64(val, bits)
/*
#if BITS_PER_LING == 32
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_32
#define hash_long(val, bits) hash_32(val, bits)
#elif BITS_PER_LONG == 64
#define hash_long(val, bits) hash_64(val, bits)
#else
#error Wordsize not 32 or 64
#endif
 */

static inline uint32_t __hash_32_generic(uint32_t val)
{
	return val * GOLDEN_RATIO_32;
}

static inline uint32_t hash_32(uint32_t val, unsigned int bits)
{
	return __hash_32_generic(val) >> (32 - bits);
}

static inline uint32_t hash_64(uint64_t val, unsigned int bits)
{
#if BITS_PER_LONG == 64
	return val * GOLDEN_RATIO_64 >> (64 - bits);
#else
	return hash_32((uint32_t)val ^ __hash_32_generic(val >> 32), bits);
#endif
}

static inline uint32_t hash_ptr(const void *ptr, unsigned int bits)
{
	return hash_long((unsigned long)ptr, bits);
}

static inline uint32_t hash32_ptr(const void *ptr)
{
	unsigned long val = (unsigned long)ptr;
#if BITS_PER_LONG == 64
	val ^= (val >> 32);
#endif
	return (uint32_t)val;
}

#endif //__HASH_H__
