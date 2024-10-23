#ifndef BIT_MAP_H
#define BIT_MAP_H

#include "yu_types.h"
#include "prng.h"
#include <string.h>

static inline int ffs_(int i)
{
	static const unsigned char table[] = { 0, 1, 2, 0, 3, 0, 0, 0, 4 };
	unsigned int a, b;
	unsigned int x = i & -i;

	a = x <= 0xffff ? (x <= 0xff ? 0 : 8) : (x <= 0xffffff ? 16 : 24);
	b = x >> a;
	if (b > 8) {
		b /= 16;
		a += 4;
	}

	return table[b] + a;
}
static inline int ffsll_(long long int i)
{
	unsigned long long int x = i & -i;

	if (x <= 0xffffffff)
		return ffs_(i);
	else
		return 32 + ffs_(i >> 32);
}

static inline int fls_(int i)
{
	i |= (i >> 1); //最高位和次高位变为1
	i |= (i >> 2); //最高和之后4位变为1
	i |= (i >> 4); //最高和之后8位变为1
	i |= (i >> 8); //最高和之后16位变为1
	i |= (i >> 16); //最高和之后32位变为1
	i++;

	if (!i)
		return 32;

	return ffs_(i) - 1;
}
static inline int flsll_(long long int i)
{
	if (((unsigned long long int)i) <= 0xffffffff)
		return fls_(i);
	else
		return 32 + fls_(i >> 32);
}

static inline int popcount_u64(unsigned long long x)
{
	//按2位: 00->00, 01/10->01, 11->10
	x = x - ((x >> 1) & 0x5555555555555555ULL);

	//按4位: [0000, 0001, 0010, 0100, 0101, 0110, 1000, 1001, 1010] -> [0000, 0001, 0010, 0011, 0100](0/1/2/3/4)
	x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);

	//按8位: [00000000, 00000001, ..., 10101001, 10101010] -> [0x00, 0x01, ..., 0x8](0~8)
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;

	//乘以0x0101010101010101，所有的8位求和都到了最高8位(最大56不会溢出)，右移56位取出最高8位
	return (x * 0x0101010101010101ULL) >> 56;
}
static inline int popcount_u32(unsigned int x)
{
	return popcount_u64((unsigned long long)x);
}

//The flat bitmap module
//first base is control, now is the nbits
typedef unsigned int fb_base;
#define BITS_START 1 //the bitmap start position, less than is the control bytes
#define LG_SIZEOF_FB LG_SIZEOF_INT

#define FB_BASE_BITS (SZ(1) << (LG_SIZEOF_FB + 3))
#define FB_NGROUPS(nbits) ((nbits) / FB_BASE_BITS + ((nbits) % FB_BASE_BITS != 0))
#define FB_LENS(nbits) (FB_NGROUPS(nbits) + BITS_START)

#define FB_CTRL_NBITS 24 //must less than FB_BASE_BITS
#define FB_GET_NBITS(fb) (fb[0] & (((fb_base)1 << FB_CTRL_NBITS) - 1))

static inline size_t fb_popcount(fb_base fb_data)
{
	if (LG_SIZEOF_FB == LG_SIZEOF_INT)
		return popcount_u32((unsigned int)fb_data);
	return popcount_u64((unsigned long long)fb_data);
}

static inline size_t fb_ffs(fb_base fb_data)
{
	if (LG_SIZEOF_FB == LG_SIZEOF_INT)
		return ffs_((unsigned int)fb_data) - 1;
	return ffsll_((unsigned long long)fb_data) - 1;
}

static inline size_t fb_fls(fb_base fb_data)
{
	if (LG_SIZEOF_FB == LG_SIZEOF_INT)
		return fls_((unsigned int)fb_data) - 1;
	return flsll_((unsigned long long)fb_data) - 1;
}

static inline fb_base *fb_init(size_t nbits)
{
	assert(nbits < ((fb_base)1 << FB_CTRL_NBITS));
	fb_base *fb = (fb_base *)calloc(FB_LENS(nbits), sizeof(fb_base));

	if (fb)
		fb[0] = nbits;

	return fb;
}

static inline void fb_init_stack(fb_base *fb, size_t nbits)
{
	assert(nbits < ((fb_base)1 << FB_CTRL_NBITS));

	memset(fb, 0, FB_LENS(nbits) * sizeof(fb_base));
	fb[0] = nbits;
}

static inline void fb_destroy(fb_base *fb)
{
	free((void *)fb);
}

static inline bool fb_empty(fb_base *fb)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(fb));
	for (size_t i = BITS_START; i < ngroups; i++) {
		if (fb[i] != 0) {
			return false;
		}
	}
	return true;
}

static inline bool fb_full(fb_base *fb)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(fb));
	size_t trailing_bits = FB_GET_NBITS(fb) % FB_BASE_BITS;
	size_t limit = (trailing_bits == 0 ? ngroups : ngroups - 1);
	for (size_t i = BITS_START; i < limit; i++) {
		if (fb[i] != ~(fb_base)0) {
			return false;
		}
	}
	if (trailing_bits == 0) {
		return true;
	}
	return fb[ngroups - 1] == ((fb_base)1 << trailing_bits) - 1;
}

//bit: 0 ~ nbits-1
static inline bool fb_get(fb_base *fb, size_t bit)
{
	assert(bit < FB_GET_NBITS(fb));
	size_t group_ind = bit / FB_BASE_BITS + BITS_START;
	size_t bit_ind = bit % FB_BASE_BITS;
	return (bool)(fb[group_ind] & ((fb_base)1 << bit_ind));
}

static inline void fb_set(fb_base *fb, size_t bit)
{
	assert(bit < FB_GET_NBITS(fb));
	size_t group_ind = bit / FB_BASE_BITS + BITS_START;
	size_t bit_ind = bit % FB_BASE_BITS;
	fb[group_ind] |= ((fb_base)1 << bit_ind);
}

static inline void fb_unset(fb_base *fb, size_t bit)
{
	assert(bit < FB_GET_NBITS(fb));
	size_t group_ind = bit / FB_BASE_BITS + BITS_START;
	size_t bit_ind = bit % FB_BASE_BITS;
	fb[group_ind] &= ~((fb_base)1 << bit_ind);
}

typedef void (*fb_group_visitor_t)(void *ctx, fb_base *fb, fb_base mask);
static inline void fb_visit_impl(fb_base *fb, fb_group_visitor_t visit, void *ctx, size_t start, size_t cnt)
{
	assert(cnt > 0);
	assert(start + cnt <= FB_GET_NBITS(fb));
	size_t group_ind = start / FB_BASE_BITS + BITS_START;
	size_t start_bit_ind = start % FB_BASE_BITS;

	// First group
	size_t first_group_cnt = (start_bit_ind + cnt > FB_BASE_BITS ? FB_BASE_BITS - start_bit_ind : cnt);
	fb_base mask = ((~(fb_base)0) >> (FB_BASE_BITS - first_group_cnt)) << start_bit_ind;
	visit(ctx, &fb[group_ind], mask);

	cnt -= first_group_cnt;
	group_ind++;
	// Middle groups
	while (cnt > FB_BASE_BITS) {
		visit(ctx, &fb[group_ind], ~(fb_base)0);
		cnt -= FB_BASE_BITS;
		group_ind++;
	}

	// Last group
	if (cnt != 0) {
		mask = (~(fb_base)0) >> (FB_BASE_BITS - cnt);
		visit(ctx, &fb[group_ind], mask);
	}
}

static inline void fb_assign_visitor(void *ctx, fb_base *fb, fb_base mask)
{
	bool val = *(bool *)ctx;
	if (val) {
		*fb |= mask;
	} else {
		*fb &= ~mask;
	}
}

// Sets the cnt bits starting at position start.  Must not have a 0 count.
static inline void fb_set_range(fb_base *fb, size_t start, size_t cnt)
{
	bool val = true;
	fb_visit_impl(fb, &fb_assign_visitor, &val, start, cnt);
}

// Unsets the cnt bits starting at position start.  Must not have a 0 count.
static inline void fb_unset_range(fb_base *fb, size_t start, size_t cnt)
{
	bool val = false;
	fb_visit_impl(fb, &fb_assign_visitor, &val, start, cnt);
}

static inline void fb_scount_visitor(void *ctx, fb_base *fb, fb_base mask)
{
	size_t *scount = (size_t *)ctx;
	*scount += fb_popcount(*fb & mask);
}

// Finds the number of set bit in the of length cnt starting at start.
static inline size_t fb_scount(fb_base *fb, size_t start, size_t cnt)
{
	size_t scount = 0;
	fb_visit_impl(fb, &fb_scount_visitor, &scount, start, cnt);
	return scount;
}

// Finds the number of unset bit in the of length cnt starting at start.
static inline size_t fb_ucount(fb_base *fb, size_t start, size_t cnt)
{
	size_t scount = fb_scount(fb, start, cnt);
	return cnt - scount;
}

//An implementation detail; find the first bit at position >= min_bit with the value val.
//Returns the number of bits in the bitmap if no such bit exists.
static inline ssize_t fb_find_impl(fb_base *fb, size_t start, bool val, bool forward)
{
	size_t nbits = FB_GET_NBITS(fb);
	assert(start < nbits);
	size_t ngroups = FB_NGROUPS(nbits);
	ssize_t group_ind = start / FB_BASE_BITS;
	size_t bit_ind = start % FB_BASE_BITS;

	fb = &fb[BITS_START];
	fb_base maybe_invert = (val ? 0 : (fb_base)-1);

	fb_base group = fb[group_ind];
	group ^= maybe_invert;
	if (forward) {
		//Only keep ones in bits bit_ind and above.
		group &= ~((1LU << bit_ind) - 1);
	} else {
		//Only keep ones in bits bit_ind and below.
		group &= ((2LU << bit_ind) - 1);
	}
	ssize_t group_ind_bound = forward ? (ssize_t)ngroups : -1;
	while (group == 0) {
		group_ind += forward ? 1 : -1;
		if (group_ind == group_ind_bound) {
			return forward ? (ssize_t)nbits : (ssize_t)-1;
		}
		group = fb[group_ind];
		group ^= maybe_invert;
	}
	assert(group != 0);
	size_t bit = forward ? fb_ffs(group) : fb_fls(group);
	size_t pos = group_ind * FB_BASE_BITS + bit;

	//The high bits of a partially filled last group are zeros, so if we're
	//looking for zeros we don't want to report an invalid result.
	if (forward && !val && pos > nbits) {
		return nbits;
	}
	return pos;
}

//Find the first set bit in the bitmap with an index >= min_bit.
//Returns the number of bits in the bitmap if no such bit exists.
static inline size_t fb_first_set_forward(fb_base *fb, size_t min_bit)
{
	return (size_t)fb_find_impl(fb, min_bit, /* val */ true, /* forward */ true);
}

// The same, but looks for an unset bit.
static inline size_t fb_first_unset_forward(fb_base *fb, size_t min_bit)
{
	return (size_t)fb_find_impl(fb, min_bit, /* val */ false, /* forward */ true);
}

//Find the last set bit in the bitmap with an index <= max_bit.
//Returns -1 if no such bit exists.
static inline ssize_t fb_first_set_backward(fb_base *fb, size_t max_bit)
{
	return fb_find_impl(fb, max_bit, /* val */ true, /* forward */ false);
}

static inline ssize_t fb_first_unset_backward(fb_base *fb, size_t max_bit)
{
	return fb_find_impl(fb, max_bit, /* val */ false, /* forward */ false);
}

//Returns whether or not we found a range.
static inline bool fb_iter_range_impl(fb_base *fb, size_t start, size_t *r_begin, size_t *r_len, bool val, bool forward)
{
	size_t nbits = FB_GET_NBITS(fb);
	assert(start < nbits);

	ssize_t next_range_begin = fb_find_impl(fb, start, val, forward);
	if ((forward && next_range_begin == (ssize_t)nbits) || (!forward && next_range_begin == (ssize_t)-1)) {
		return false;
	}

	//Half open range; the set bits are [begin, end).
	ssize_t next_range_end = fb_find_impl(fb, next_range_begin, !val, forward);
	if (forward) {
		*r_begin = next_range_begin;
		*r_len = next_range_end - next_range_begin;
	} else {
		*r_begin = next_range_end + 1;
		*r_len = next_range_begin - next_range_end;
	}
	return true;
}

//Used to iterate through ranges of set bits.
static inline bool fb_set_srange_forward(fb_base *fb, size_t start, size_t *r_begin, size_t *r_len)
{
	return fb_iter_range_impl(fb, start, r_begin, r_len, /* val */ true, /* forward */ true);
}

//The same as fb_set_srange_forward, but searches backwards from start rather than
//forwards.  (The position returned is still the earliest bit in the range).
static inline bool fb_set_srange_backward(fb_base *fb, size_t start, size_t *r_begin, size_t *r_len)
{
	return fb_iter_range_impl(fb, start, r_begin, r_len, /* val */ true, /* forward */ false);
}

//Similar to fb_set_srange_forward, but searches for unset bits.
static inline bool fb_unset_srange_forward(fb_base *fb, size_t start, size_t *r_begin, size_t *r_len)
{
	return fb_iter_range_impl(fb, start, r_begin, r_len, /* val */ false, /* forward */ true);
}

//Similar to fb_set_srange_backward, but searches for unset bits.
static inline bool fb_unset_srange_backward(fb_base *fb, size_t start, size_t *r_begin, size_t *r_len)
{
	return fb_iter_range_impl(fb, start, r_begin, r_len, /* val */ false, /* forward */ false);
}

static inline size_t fb_range_longest_impl(fb_base *fb, bool val)
{
	size_t nbits = FB_GET_NBITS(fb);
	size_t begin = 0;
	size_t longest_len = 0;
	size_t len = 0;

	while (begin < nbits && fb_iter_range_impl(fb, begin, &begin, &len, val, /* forward */ true)) {
		if (len > longest_len) {
			longest_len = len;
		}
		begin += len;
	}
	return longest_len;
}

static inline size_t fb_set_srange_longest(fb_base *fb)
{
	return fb_range_longest_impl(fb, /* val */ true);
}

static inline size_t fb_unset_srange_longest(fb_base *fb)
{
	return fb_range_longest_impl(fb, /* val */ false);
}

//Initializes each bit of dst with the bitwise-AND of the corresponding bits of
//src1 and src2.  All bitmaps must be the same size.
static inline void fb_bit_and(fb_base *dst, fb_base *src1, fb_base *src2)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(dst));
	for (size_t i = BITS_START; i < ngroups; i++) {
		dst[i] = src1[i] & src2[i];
	}
}

//Like fb_bit_and, but with bitwise-OR.
static inline void fb_bit_or(fb_base *dst, fb_base *src1, fb_base *src2)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(dst));
	for (size_t i = BITS_START; i < ngroups; i++) {
		dst[i] = src1[i] | src2[i];
	}
}

//Initializes dst bit i to the negation of source bit i.
static inline void fb_bit_not(fb_base *dst, fb_base *src)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(dst));
	for (size_t i = BITS_START; i < ngroups; i++) {
		dst[i] = ~src[i];
	}
}
#endif