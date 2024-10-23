#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "bit_map.h"
#include "prng.h"
#include "../test.h"

#define NBITS_TAB                                                                                                                                    \
	NB(1)                                                                                                                                        \
	NB(2)                                                                                                                                        \
	NB(3)                                                                                                                                        \
	NB(4)                                                                                                                                        \
	NB(5)                                                                                                                                        \
	NB(6) NB(7) NB(8) NB(9) NB(10) NB(11) NB(12) NB(13) NB(14) NB(15) NB(16) NB(17) NB(18) NB(19) NB(20) NB(21) NB(22) NB(23) NB(24) NB(25)      \
		NB(26) NB(27) NB(28) NB(29) NB(30) NB(31) NB(32) NB(33) NB(34) NB(35) NB(36) NB(37) NB(38) NB(39) NB(40) NB(41) NB(42) NB(43) NB(44) \
			NB(45) NB(46) NB(47) NB(48) NB(49) NB(50) NB(51) NB(52) NB(53) NB(54) NB(55) NB(56) NB(57) NB(58) NB(59) NB(60) NB(61)       \
				NB(62) NB(63) NB(64) NB(65) NB(66) NB(67) NB(126) NB(127) NB(128) NB(129) NB(130) NB(254) NB(255) NB(256) NB(257)    \
					NB(258) NB(510) NB(511) NB(512) NB(513) NB(514) NB(1022) NB(1023) NB(1024) NB(1025) NB(1026) NB(2048)        \
						NB(4094) NB(4095) NB(4096) NB(4097) NB(4098) NB(8192) NB(16384)

static void do_test_init(size_t nbits)
{
	fb_base *fb = fb_init(nbits);
	for (size_t i = 0; i < nbits; i++) {
		expect_false(fb_get(fb, i), "bitmap should start empty");
	}
	fb_destroy(fb);
}

TEST_BEGIN(test_fb_init)
{
#define NB(nbits) do_test_init(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static void do_test_get_set_unset(size_t nbits)
{
	fb_base *fb = fb_init(nbits);
	//Set the bits divisible by 3.
	for (size_t i = 0; i < nbits; i++) {
		if (i % 3 == 0) {
			fb_set(fb, i);
		}
	}
	//Check them.
	for (size_t i = 0; i < nbits; i++) {
		expect_b_eq(i % 3 == 0, fb_get(fb, i), "Unexpected bit at position %zu", i);
	}
	//Unset those divisible by 5.
	for (size_t i = 0; i < nbits; i++) {
		if (i % 5 == 0) {
			fb_unset(fb, i);
		}
	}
	//Check them.
	for (size_t i = 0; i < nbits; i++) {
		expect_b_eq(i % 3 == 0 && i % 5 != 0, fb_get(fb, i), "Unexpected bit at position %zu", i);
	}
	fb_destroy(fb);
}

TEST_BEGIN(test_get_set_unset)
{
#define NB(nbits) do_test_get_set_unset(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static ssize_t find_3_5_compute(ssize_t i, size_t nbits, bool bit, bool forward)
{
	for (; i < (ssize_t)nbits && i >= 0; i += (forward ? 1 : -1)) {
		bool expected_bit = i % 3 == 0 || i % 5 == 0;
		if (expected_bit == bit) {
			return i;
		}
	}
	return forward ? (ssize_t)nbits : (ssize_t)-1;
}

static void do_test_search_simple(size_t nbits)
{
	fb_base *fb = fb_init(nbits);

	//We pick multiples of 3 or 5.
	for (size_t i = 0; i < nbits; i++) {
		if (i % 3 == 0) {
			fb_set(fb, i);
		}
		//This tests double-setting a little, too.
		if (i % 5 == 0) {
			fb_set(fb, i);
		}
	}
	for (size_t i = 0; i < nbits; i++) {
		size_t ffs_compute = find_3_5_compute(i, FB_GET_NBITS(fb), true, true);
		size_t ffs_search = fb_first_set_forward(fb, i);
		expect_zu_eq(ffs_compute, ffs_search, "ffs mismatch at %zu", i);

		ssize_t fls_compute = find_3_5_compute(i, FB_GET_NBITS(fb), true, false);
		size_t fls_search = fb_first_set_backward(fb, i);
		expect_zu_eq(fls_compute, fls_search, "fls mismatch at %zu", i);

		size_t ffu_compute = find_3_5_compute(i, FB_GET_NBITS(fb), false, true);
		size_t ffu_search = fb_first_unset_forward(fb, i);
		expect_zu_eq(ffu_compute, ffu_search, "ffu mismatch at %zu", i);

		size_t flu_compute = find_3_5_compute(i, FB_GET_NBITS(fb), false, false);
		size_t flu_search = fb_first_unset_backward(fb, i);
		expect_zu_eq(flu_compute, flu_search, "flu mismatch at %zu", i);
	}

	fb_destroy(fb);
}

TEST_BEGIN(test_search_simple)
{
#define NB(nbits) do_test_search_simple(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static void expect_exhaustive_results(fb_base *mostly_full, fb_base *mostly_empty, size_t nbits, size_t special_bit, size_t position)
{
	if (position < special_bit) {
		expect_zu_eq(special_bit, fb_first_set_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(-1, fb_first_set_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(position, fb_first_unset_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position, fb_first_unset_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);

		expect_zu_eq(position, fb_first_set_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position, fb_first_set_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(special_bit, fb_first_unset_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(-1, fb_first_unset_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
	} else if (position == special_bit) {
		expect_zu_eq(special_bit, fb_first_set_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(special_bit, fb_first_set_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(position + 1, fb_first_unset_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position - 1, fb_first_unset_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);

		expect_zu_eq(position + 1, fb_first_set_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position - 1, fb_first_set_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(position, fb_first_unset_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position, fb_first_unset_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
	} else {
		//position > special_bit.
		expect_zu_eq(nbits, fb_first_set_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(special_bit, fb_first_set_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(position, fb_first_unset_forward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position, fb_first_unset_backward(mostly_empty, position), "mismatch at %zu, %zu", position, special_bit);

		expect_zu_eq(position, fb_first_set_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(position, fb_first_set_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zu_eq(nbits, fb_first_unset_forward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
		expect_zd_eq(special_bit, fb_first_unset_backward(mostly_full, position), "mismatch at %zu, %zu", position, special_bit);
	}
}

static void do_test_search_exhaustive(size_t nbits)
{
	//This test is quadratic; let's not get too big.
	if (nbits > 1000) {
		return;
	}

	fb_base *empty = fb_init(nbits);
	fb_base *full = fb_init(nbits);
	fb_set_range(full, 0, nbits);

	for (size_t i = 0; i < nbits; i++) {
		fb_set(empty, i);
		fb_unset(full, i);

		for (size_t j = 0; j < nbits; j++) {
			expect_exhaustive_results(full, empty, nbits, i, j);
		}
		fb_unset(empty, i);
		fb_set(full, i);
	}

	fb_destroy(empty);
	fb_destroy(full);
}

TEST_BEGIN(test_search_exhaustive)
{
#define NB(nbits) do_test_search_exhaustive(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

TEST_BEGIN(test_range_simple)
{
	//Just pick a constant big enough to have nontrivial middle sizes, and
	//big enough that usages of things like weirdnum (below) near the
	//beginning fit comfortably into the beginning of the bitmap.
	size_t nbits = 64 * 10;
	fb_base *fb = fb_init(nbits);

	for (size_t i = 0; i < nbits; i++) {
		if (i % 2 == 0) {
			fb_set_range(fb, i, 1);
		}
	}
	for (size_t i = 0; i < nbits; i++) {
		expect_b_eq(i % 2 == 0, fb_get(fb, i), "mismatch at position %zu", i);
	}
	fb_set_range(fb, 0, nbits / 2);
	fb_unset_range(fb, nbits / 2, nbits / 2);
	for (size_t i = 0; i < nbits; i++) {
		expect_b_eq(i < nbits / 2, fb_get(fb, i), "mismatch at position %zu", i);
	}

	static const size_t weirdnum = 7;
	fb_set_range(fb, 0, nbits);
	fb_unset_range(fb, weirdnum, FB_BASE_BITS + weirdnum);
	for (size_t i = 0; i < nbits; i++) {
		expect_b_eq(weirdnum <= i && i <= 2 * weirdnum + FB_BASE_BITS - 1, !fb_get(fb, i), "mismatch at position %zu", i);
	}
	fb_destroy(fb);
}
TEST_END

static void do_test_empty_full_exhaustive(size_t nbits)
{
	fb_base *empty = fb_init(nbits);
	fb_base *full = fb_init(nbits);
	fb_set_range(full, 0, nbits);

	expect_true(fb_full(full), "%s", "");
	expect_false(fb_empty(full), "%s", "");
	expect_false(fb_full(empty), "%s", "");
	expect_true(fb_empty(empty), "%s", "");

	for (size_t i = 0; i < nbits; i++) {
		fb_set(empty, i);
		fb_unset(full, i);

		expect_false(fb_empty(empty), "error at bit %zu", i);
		if (nbits != 1) {
			expect_false(fb_full(empty), "error at bit %zu", i);
			expect_false(fb_empty(full), "error at bit %zu", i);
		} else {
			expect_true(fb_full(empty), "error at bit %zu", i);
			expect_true(fb_empty(full), "error at bit %zu", i);
		}
		expect_false(fb_full(full), "error at bit %zu", i);

		fb_unset(empty, i);
		fb_set(full, i);
	}

	fb_destroy(empty);
	fb_destroy(full);
}

TEST_BEGIN(test_empty_full)
{
#define NB(nbits) do_test_empty_full_exhaustive(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

//This tests both iter_range and the longest range functionality, which is
//built closely on top of it.
TEST_BEGIN(test_iter_range_simple)
{
	size_t set_limit = 30;
	size_t nbits = 100;
	fb_base fb[FB_LENS(100)];

	fb_init_stack(fb, nbits);

	//Failing to initialize these can lead to build failures with -Wall;
	//the compiler can't prove that they're set.
	size_t begin = (size_t)-1;
	size_t len = (size_t)-1;
	bool result;

	//A set of checks with only the first set_limit bits *set*.
	fb_set_range(fb, 0, set_limit);
	expect_zu_eq(set_limit, fb_set_srange_longest(fb), "Incorrect longest set range");
	expect_zu_eq(nbits - set_limit, fb_unset_srange_longest(fb), "Incorrect longest unset range");
	for (size_t i = 0; i < set_limit; i++) {
		result = fb_set_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(i, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(set_limit - i, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(set_limit, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(nbits - set_limit, len, "Incorrect len at %zu", i);

		result = fb_set_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(0, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(i + 1, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_backward(fb, i, &begin, &len);
		expect_false(result, "Should not have found a range at %zu", i);
	}
	for (size_t i = set_limit; i < nbits; i++) {
		result = fb_set_srange_forward(fb, i, &begin, &len);
		expect_false(result, "Should not have found a range at %zu", i);

		result = fb_unset_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(i, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(nbits - i, len, "Incorrect len at %zu", i);

		result = fb_set_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(0, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(set_limit, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(set_limit, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(i - set_limit + 1, len, "Incorrect len at %zu", i);
	}

	//A set of checks with only the first set_limit bits *unset*.
	fb_unset_range(fb, 0, set_limit);
	fb_set_range(fb, set_limit, nbits - set_limit);
	expect_zu_eq(nbits - set_limit, fb_set_srange_longest(fb), "Incorrect longest set range");
	expect_zu_eq(set_limit, fb_unset_srange_longest(fb), "Incorrect longest unset range");
	for (size_t i = 0; i < set_limit; i++) {
		result = fb_set_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(set_limit, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(nbits - set_limit, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(i, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(set_limit - i, len, "Incorrect len at %zu", i);

		result = fb_set_srange_backward(fb, i, &begin, &len);
		expect_false(result, "Should not have found a range at %zu", i);

		result = fb_unset_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should not have found a range at %zu", i);
		expect_zu_eq(0, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(i + 1, len, "Incorrect len at %zu", i);
	}
	for (size_t i = set_limit; i < nbits; i++) {
		result = fb_set_srange_forward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(i, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(nbits - i, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_forward(fb, i, &begin, &len);
		expect_false(result, "Should not have found a range at %zu", i);

		result = fb_set_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(set_limit, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(i - set_limit + 1, len, "Incorrect len at %zu", i);

		result = fb_unset_srange_backward(fb, i, &begin, &len);
		expect_true(result, "Should have found a range at %zu", i);
		expect_zu_eq(0, begin, "Incorrect begin at %zu", i);
		expect_zu_eq(set_limit, len, "Incorrect len at %zu", i);
	}
}
TEST_END

//Doing this bit-by-bit is too slow for a real implementation, but for testing code,
//it's easy to get right.  In the exhaustive tests, we'll compare the
//(fast but tricky) real implementation against the (slow but simple) testing one.

static bool fb_iter_simple(fb_base *fb, size_t nbits, size_t start, size_t *r_begin, size_t *r_len, bool val, bool forward)
{
	ssize_t stride = (forward ? (ssize_t)1 : (ssize_t)-1);
	ssize_t range_begin = (ssize_t)start;
	for (; range_begin != (ssize_t)nbits && range_begin != -1; range_begin += stride) {
		if (fb_get(fb, range_begin) == val) {
			ssize_t range_end = range_begin;
			for (; range_end != (ssize_t)nbits && range_end != -1; range_end += stride) {
				if (fb_get(fb, range_end) != val) {
					break;
				}
			}
			if (forward) {
				*r_begin = range_begin;
				*r_len = range_end - range_begin;
			} else {
				*r_begin = range_end + 1;
				*r_len = range_begin - range_end;
			}
			return true;
		}
	}
	return false;
}

//Similar, but for finding longest ranges.
static size_t fb_range_longest_simple(fb_base *fb, size_t nbits, bool val)
{
	size_t longest_so_far = 0;
	for (size_t begin = 0; begin < nbits; begin++) {
		if (fb_get(fb, begin) != val) {
			continue;
		}
		size_t end = begin + 1;
		for (; end < nbits; end++) {
			if (fb_get(fb, end) != val) {
				break;
			}
		}
		if (end - begin > longest_so_far) {
			longest_so_far = end - begin;
		}
	}
	return longest_so_far;
}

static void expect_iter_results_at(fb_base *fb, size_t pos, bool val, bool forward)
{
	bool iter_res;
	size_t iter_begin = 0;
	size_t iter_len = 0;
	if (val) {
		if (forward) {
			iter_res = fb_set_srange_forward(fb, pos, &iter_begin, &iter_len);
		} else {
			iter_res = fb_set_srange_backward(fb, pos, &iter_begin, &iter_len);
		}
	} else {
		if (forward) {
			iter_res = fb_unset_srange_forward(fb, pos, &iter_begin, &iter_len);
		} else {
			iter_res = fb_unset_srange_backward(fb, pos, &iter_begin, &iter_len);
		}
	}

	bool simple_iter_res;
	size_t simple_iter_begin = 0;
	size_t simple_iter_len = 0;
	simple_iter_res = fb_iter_simple(fb, FB_GET_NBITS(fb), pos, &simple_iter_begin, &simple_iter_len, val, forward);

	expect_b_eq(iter_res, simple_iter_res, "Result mismatch at %zu", pos);
	if (iter_res && simple_iter_res) {
		assert_zu_eq(iter_begin, simple_iter_begin, "Begin mismatch at %zu", pos);
		expect_zu_eq(iter_len, simple_iter_len, "Length mismatch at %zu", pos);
	}
}

static void expect_iter_results(fb_base *fb)
{
	for (size_t i = 0; i < FB_GET_NBITS(fb); i++) {
		expect_iter_results_at(fb, i, false, false);
		expect_iter_results_at(fb, i, false, true);
		expect_iter_results_at(fb, i, true, false);
		expect_iter_results_at(fb, i, true, true);
	}
	expect_zu_eq(fb_range_longest_simple(fb, FB_GET_NBITS(fb), true), fb_set_srange_longest(fb), "Longest range mismatch");
	expect_zu_eq(fb_range_longest_simple(fb, FB_GET_NBITS(fb), false), fb_unset_srange_longest(fb), "Longest range mismatch");
}

static void set_pattern_3(fb_base *fb, bool zero_val)
{
	for (size_t i = 0; i < FB_GET_NBITS(fb); i++) {
		if ((i % 6 < 3 && zero_val) || (i % 6 >= 3 && !zero_val)) {
			fb_set(fb, i);
		} else {
			fb_unset(fb, i);
		}
	}
}

static void do_test_iter_range_exhaustive(size_t nbits)
{
	//This test is also pretty slow.
	if (nbits > 1000) {
		return;
	}
	fb_base *fb = fb_init(nbits);

	set_pattern_3(fb, /*zero_val*/ true);
	expect_iter_results(fb);

	set_pattern_3(fb, /*zero_val*/ false);
	expect_iter_results(fb);

	fb_set_range(fb, 0, nbits);
	fb_unset_range(fb, 0, nbits / 2 == 0 ? 1 : nbits / 2);
	expect_iter_results(fb);

	fb_unset_range(fb, 0, nbits);
	fb_set_range(fb, 0, nbits / 2 == 0 ? 1 : nbits / 2);
	expect_iter_results(fb);

	fb_destroy(fb);
}

//Like test_iter_range_simple, this tests both iteration and longest-range computation.
TEST_BEGIN(test_iter_range_exhaustive)
{
#define NB(nbits) do_test_iter_range_exhaustive(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

//If all set bits in the bitmap are contiguous, in [set_start, set_end),
//returns the number of set bits in [scount_start, scount_end).
static size_t scount_contiguous(size_t set_start, size_t set_end, size_t scount_start, size_t scount_end)
{
	//No overlap.
	if (set_end <= scount_start || scount_end <= set_start) {
		return 0;
	}
	//set range contains scount range
	if (set_start <= scount_start && set_end >= scount_end) {
		return scount_end - scount_start;
	}
	//scount range contains set range.
	if (scount_start <= set_start && scount_end >= set_end) {
		return set_end - set_start;
	}
	//Partial overlap, with set range starting first.
	if (set_start < scount_start && set_end < scount_end) {
		return set_end - scount_start;
	}
	//Partial overlap, with scount range starting first.
	if (scount_start < set_start && scount_end < set_end) {
		return scount_end - set_start;
	}

	unreachable();
}

static size_t ucount_contiguous(size_t set_start, size_t set_end, size_t ucount_start, size_t ucount_end)
{
	//No overlap.
	if (set_end <= ucount_start || ucount_end <= set_start) {
		return ucount_end - ucount_start;
	}
	//set range contains ucount range
	if (set_start <= ucount_start && set_end >= ucount_end) {
		return 0;
	}
	//ucount range contains set range.
	if (ucount_start <= set_start && ucount_end >= set_end) {
		return (ucount_end - ucount_start) - (set_end - set_start);
	}
	//Partial overlap, with set range starting first.
	if (set_start < ucount_start && set_end < ucount_end) {
		return ucount_end - set_end;
	}
	//Partial overlap, with ucount range starting first.
	if (ucount_start < set_start && ucount_end < set_end) {
		return set_start - ucount_start;
	}

	unreachable();
}

static void expect_count_match_contiguous(fb_base *fb, size_t nbits, size_t set_start, size_t set_end)
{
	for (size_t i = 0; i < nbits; i++) {
		for (size_t j = i + 1; j <= nbits; j++) {
			size_t cnt = j - i;
			size_t scount_expected = scount_contiguous(set_start, set_end, i, j);
			size_t scount_computed = fb_scount(fb, i, cnt);
			expect_zu_eq(scount_expected, scount_computed,
				     "fb_scount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with bits set in [%zu, %zu)",
				     nbits, i, cnt, set_start, set_end);

			size_t ucount_expected = ucount_contiguous(set_start, set_end, i, j);
			size_t ucount_computed = fb_ucount(fb, i, cnt);
			assert_zu_eq(ucount_expected, ucount_computed,
				     "fb_ucount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with bits set in [%zu, %zu)",
				     nbits, i, cnt, set_start, set_end);
		}
	}
}

static void do_test_count_contiguous(size_t nbits)
{
	fb_base *fb = fb_init(nbits);

	expect_count_match_contiguous(fb, nbits, 0, 0);
	for (size_t i = 0; i < nbits; i++) {
		fb_set(fb, i);
		expect_count_match_contiguous(fb, nbits, 0, i + 1);
	}

	for (size_t i = 0; i < nbits; i++) {
		fb_unset(fb, i);
		expect_count_match_contiguous(fb, nbits, i + 1, nbits);
	}

	fb_destroy(fb);
}

TEST_BEGIN(test_count_contiguous_simple)
{
	enum { nbits = 300 };
	fb_base fb[FB_LENS(nbits)];
	fb_init_stack(fb, nbits);
	//Just an arbitrary number.
	size_t start = 23;

	fb_set_range(fb, start, 30 - start);
	expect_count_match_contiguous(fb, nbits, start, 30);

	fb_set_range(fb, start, 40 - start);
	expect_count_match_contiguous(fb, nbits, start, 40);

	fb_set_range(fb, start, 70 - start);
	expect_count_match_contiguous(fb, nbits, start, 70);

	fb_set_range(fb, start, 120 - start);
	expect_count_match_contiguous(fb, nbits, start, 120);

	fb_set_range(fb, start, 150 - start);
	expect_count_match_contiguous(fb, nbits, start, 150);

	fb_set_range(fb, start, 200 - start);
	expect_count_match_contiguous(fb, nbits, start, 200);

	fb_set_range(fb, start, 290 - start);
	expect_count_match_contiguous(fb, nbits, start, 290);
}
TEST_END

TEST_BEGIN(test_count_contiguous)
{
#define NB(nbits)                                             \
	/*This test is *particularly* slow in debug builds.*/ \
	if (nbits < 300 || nbits < 150) {                     \
		do_test_count_contiguous(nbits);              \
	};
	NBITS_TAB
#undef NB
}
TEST_END

static void expect_count_match_alternating(fb_base *fb_even, fb_base *fb_odd, size_t nbits)
{
	for (size_t i = 0; i < nbits; i++) {
		for (size_t j = i + 1; j <= nbits; j++) {
			size_t cnt = j - i;
			size_t odd_scount = cnt / 2 + (size_t)(cnt % 2 == 1 && i % 2 == 1);
			size_t odd_scount_computed = fb_scount(fb_odd, i, j - i);
			assert_zu_eq(odd_scount, odd_scount_computed,
				     "fb_scount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with alternating bits set.",
				     nbits, i, j - i);

			size_t odd_ucount = cnt / 2 + (size_t)(cnt % 2 == 1 && i % 2 == 0);
			size_t odd_ucount_computed = fb_ucount(fb_odd, i, j - i);
			assert_zu_eq(odd_ucount, odd_ucount_computed,
				     "fb_ucount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with alternating bits set.",
				     nbits, i, j - i);

			size_t even_scount = cnt / 2 + (size_t)(cnt % 2 == 1 && i % 2 == 0);
			size_t even_scount_computed = fb_scount(fb_even, i, j - i);
			assert_zu_eq(even_scount, even_scount_computed,
				     "fb_scount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with alternating bits set.",
				     nbits, i, j - i);

			size_t even_ucount = cnt / 2 + (size_t)(cnt % 2 == 1 && i % 2 == 1);
			size_t even_ucount_computed = fb_ucount(fb_even, i, j - i);
			assert_zu_eq(even_ucount, even_ucount_computed,
				     "fb_ucount error with nbits=%zu, start=%zu, "
				     "cnt=%zu, with alternating bits set.",
				     nbits, i, j - i);
		}
	}
}

static void do_test_count_alternating(size_t nbits)
{
	if (nbits > 1000) {
		return;
	}
	fb_base *fb_even = fb_init(nbits);
	fb_base *fb_odd = fb_init(nbits);

	for (size_t i = 0; i < nbits; i++) {
		if (i % 2 == 0) {
			fb_set(fb_even, i);
		} else {
			fb_set(fb_odd, i);
		}
	}

	expect_count_match_alternating(fb_even, fb_odd, nbits);

	fb_destroy(fb_even);
	fb_destroy(fb_odd);
}

TEST_BEGIN(test_count_alternating)
{
#define NB(nbits) do_test_count_alternating(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static void do_test_bit_op(size_t nbits, bool (*op)(bool a, bool b), void (*fb_op)(fb_base *dst, fb_base *src1, fb_base *src2))
{
	fb_base *fb1 = fb_init(nbits);
	fb_base *fb2 = fb_init(nbits);
	fb_base *fb_result = fb_init(nbits);

	//Just two random numbers.
	const uint64_t prng_init1 = (uint64_t)0X4E9A9DE6A35691CDULL;
	const uint64_t prng_init2 = (uint64_t)0X7856E396B063C36EULL;

	uint64_t prng1 = prng_init1;
	uint64_t prng2 = prng_init2;

	for (size_t i = 0; i < nbits; i++) {
		bool bit1 = ((prng1 & (1ULL << (i % 64))) != 0);
		bool bit2 = ((prng2 & (1ULL << (i % 64))) != 0);

		if (bit1) {
			fb_set(fb1, i);
		}
		if (bit2) {
			fb_set(fb2, i);
		}

		if (i % 64 == 0) {
			prng1 = prng_state_next_u64(prng1);
			prng2 = prng_state_next_u64(prng2);
		}
	}

	fb_op(fb_result, fb1, fb2);

	//Reset the prngs to replay them.
	prng1 = prng_init1;
	prng2 = prng_init2;

	for (size_t i = 0; i < nbits; i++) {
		bool bit1 = ((prng1 & (1ULL << (i % 64))) != 0);
		bool bit2 = ((prng2 & (1ULL << (i % 64))) != 0);

		//Original bitmaps shouldn't change.
		expect_b_eq(bit1, fb_get(fb1, i), "difference at bit %zu", i);
		expect_b_eq(bit2, fb_get(fb2, i), "difference at bit %zu", i);

		//New one should be bitwise and.
		expect_b_eq(op(bit1, bit2), fb_get(fb_result, i), "difference at bit %zu", i);

		//Update the same way we did last time.
		if (i % 64 == 0) {
			prng1 = prng_state_next_u64(prng1);
			prng2 = prng_state_next_u64(prng2);
		}
	}

	fb_destroy(fb1);
	fb_destroy(fb2);
	fb_destroy(fb_result);
}

static bool binary_and(bool a, bool b)
{
	return a & b;
}

static void do_test_bit_and(size_t nbits)
{
	do_test_bit_op(nbits, &binary_and, &fb_bit_and);
}

TEST_BEGIN(test_bit_and)
{
#define NB(nbits) do_test_bit_and(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static bool binary_or(bool a, bool b)
{
	return a | b;
}

static void do_test_bit_or(size_t nbits)
{
	do_test_bit_op(nbits, &binary_or, &fb_bit_or);
}

TEST_BEGIN(test_bit_or)
{
#define NB(nbits) do_test_bit_or(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static bool binary_not(bool a, bool b)
{
	(void)b;
	return !a;
}

static void fb_bit_not_shim(fb_base *dst, fb_base *src1, fb_base *src2)
{
	(void)src2;
	fb_bit_not(dst, src1);
}

static void do_test_bit_not(size_t nbits)
{
	do_test_bit_op(nbits, &binary_not, &fb_bit_not_shim);
}

TEST_BEGIN(test_bit_not)
{
#define NB(nbits) do_test_bit_not(nbits);
	NBITS_TAB
#undef NB
}
TEST_END

static void fb_printf(fb_base *fb)
{
	size_t ngroups = FB_LENS(FB_GET_NBITS(fb));

	for (size_t i = 0; i < ngroups; i++)
		printf("0x%08x ", fb[i]);
	printf("\n");
}

TEST_BEGIN(test_fb_yu)
{
	size_t begin, len;
	size_t ret;
	size_t nbits = 60;
	fb_base *fb = fb_init(nbits);

	fb_set(fb, 5);
	fb_printf(fb);
	fb_set(fb, 6);
	fb_unset(fb, 5);
	fb_printf(fb);

	fb_set_range(fb, 10, 25);
	fb_printf(fb);
	fb_set_srange_forward(fb, 10, &begin, &len);
	printf("%ld %ld ", begin, len);
	fb_printf(fb);
	fb_set_srange_backward(fb, 35, &begin, &len);
	printf("%ld %ld ", begin, len);
	fb_printf(fb);
	ret = fb_first_set_forward(fb, 0);
	printf("%ld ", ret);
	fb_printf(fb);
	ret = fb_first_set_backward(fb, 50);
	printf("%ld ", ret);
	fb_printf(fb);
	ret = fb_first_unset_backward(fb, 10);
	printf("%ld ", ret);
	fb_printf(fb);
	ret = fb_first_unset_forward(fb, 10);
	printf("%ld ", ret);
	fb_printf(fb);
	ret = fb_set_srange_longest(fb);
	printf("%ld ", ret);
	fb_printf(fb);

	fb_unset_range(fb, 10, 15);
	fb_printf(fb);
	fb_unset_srange_forward(fb, 10, &begin, &len);
	printf("%ld %ld ", begin, len);
	fb_printf(fb);
	fb_unset_srange_backward(fb, 25, &begin, &len);
	printf("%ld %ld ", begin, len);
	fb_printf(fb);
	ret = fb_unset_srange_longest(fb);
	printf("%ld ", ret);
	fb_printf(fb);

	fb_destroy(fb);
}
TEST_END

int main(void)
{
	return test(test_fb_yu, test_fb_init, test_get_set_unset, test_search_simple, test_search_exhaustive, test_range_simple, test_empty_full,
		    test_iter_range_simple, test_iter_range_exhaustive, test_count_contiguous_simple, test_count_contiguous, test_count_alternating,
		    test_bit_and, test_bit_or, test_bit_not);
}
