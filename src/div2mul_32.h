#ifndef DIV_2_MUL_H
#define DIV_2_MUL_H

#include "yu_types.h"

// !!!!!!just a approximate value, equal or more than one

// This module does the division that computes the index of a region in a slab,
// given its offset relative to the base.
// That is, given a divisor d, an n = i * d (all integers), we'll return i.
// We do some pre-computation to do this more quickly than a CPU division instruction.
// We bound n < 2^32, and don't support dividing by one.

// equal: n/d mod b = n*x mod b = c, we need to get x; this is to get Multiplication inverse
//here b = 2^32, n/d = n * (2^32 / d) / 2^32, just a approximate value, equal or more than one
//this can use to bucket sort

typedef struct {
	uint32_t magic;
	//uint32_t divisor;
} div_info_s;

static inline void div_init(div_info_s *div_info, uint32_t divisor)
{
	assert(divisor != 0 && divisor != 1);

	uint64_t two_to_k = (uint64_t)1 << 32;

	div_info->magic = (uint32_t)(two_to_k / divisor) + (two_to_k % divisor != 0);
}

static inline uint32_t div_compute(div_info_s *div_info, uint32_t n)
{
	assert(n <= (uint32_t)-1);

	return ((uint64_t)n * (uint64_t)div_info->magic) >> 32;
}

#endif
