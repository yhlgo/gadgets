#include <stdlib.h>

#include "../test.h"
#include "cuckoo_hash.h"

TEST_BEGIN(test_new_delete)
{
	ckh_s ckh;

	expect_true(ckh_new(&ckh, 2, ckh_string_hash, ckh_string_keycomp, ckh_malloc_clean, ckh_free, nullptr), "Unexpected ckh_new() error");
	ckh_delete(&ckh);

	expect_true(ckh_new(&ckh, 3, ckh_pointer_hash, ckh_pointer_keycomp, ckh_malloc_clean, ckh_free, nullptr), "Unexpected ckh_new() error");
	ckh_delete(&ckh);
}
TEST_END

TEST_BEGIN(test_count_insert_search_remove)
{
	ckh_s ckh;
	const char *strs[] = { "a string", "A string", "a string.", "A string." };
	const char *missing = "A string not in the hash table.";
	size_t i;

	expect_true(ckh_new(&ckh, 2, ckh_string_hash, ckh_string_keycomp, ckh_malloc_clean, ckh_free, nullptr), "Unexpected ckh_new() error");
	expect_zu_eq(ckh_count(&ckh), 0, "ckh_count() should return %zu, but it returned %zu", SZ(0), ckh_count(&ckh));

	/* Insert. */
	for (i = 0; i < sizeof(strs) / sizeof(const char *); i++) {
		ckh_insert(&ckh, strs[i], strs[i]);
		expect_zu_eq(ckh_count(&ckh), i + 1, "ckh_count() should return %zu, but it returned %zu", i + 1, ckh_count(&ckh));
	}

	/* Search. */
	for (i = 0; i < sizeof(strs) / sizeof(const char *); i++) {
		void *kp, *vp;

		expect_true(ckh_search(&ckh, strs[i], &kp, &vp), "Unexpected ckh_search() error");

		expect_ptr_eq((void *)strs[i], (void *)kp, "Key mismatch, i=%zu", i);
		expect_ptr_eq((void *)strs[i], (void *)vp, "Value mismatch, i=%zu", i);
	}
	expect_false(ckh_search(&ckh, missing, NULL, NULL), "Unexpected ckh_search() success");

	/* Remove. */
	for (i = 0; i < sizeof(strs) / sizeof(const char *); i++) {
		void *kp, *vp;

		expect_true(ckh_remove(&ckh, strs[i], &kp, &vp), "Unexpected ckh_remove() error");

		expect_ptr_eq((void *)strs[i], (void *)kp, "Key mismatch, i=%zu", i);
		expect_ptr_eq((void *)strs[i], (void *)vp, "Value mismatch, i=%zu", i);
		expect_zu_eq(ckh_count(&ckh), sizeof(strs) / sizeof(const char *) - i - 1, "ckh_count() should return %zu, but it returned %zu",
			     sizeof(strs) / sizeof(const char *) - i - 1, ckh_count(&ckh));
	}

	ckh_delete(&ckh);
}
TEST_END

TEST_BEGIN(test_insert_iter_remove)
{
#define NITEMS SZ(5000)
	ckh_s ckh;
	void **p[NITEMS];
	void *q, *r;
	size_t i;

	expect_true(ckh_new(&ckh, 2, ckh_pointer_hash, ckh_pointer_keycomp, ckh_malloc_clean, ckh_free, nullptr), "Unexpected ckh_new() error");

	for (i = 0; i < NITEMS; i++) {
		p[i] = malloc(i + 1);
		expect_ptr_not_null(p[i], "Unexpected mallocx() failure");
	}

	for (i = 0; i < NITEMS; i++) {
		size_t j;

		for (j = i; j < NITEMS; j++) {
			expect_true(ckh_insert(&ckh, p[j], p[j]), "Unexpected ckh_insert() failure");
			expect_true(ckh_search(&ckh, p[j], &q, &r), "Unexpected ckh_search() failure");
			expect_ptr_eq(p[j], q, "Key pointer mismatch");
			expect_ptr_eq(p[j], r, "Value pointer mismatch");
		}

		expect_zu_eq(ckh_count(&ckh), NITEMS, "ckh_count() should return %zu, but it returned %zu", NITEMS, ckh_count(&ckh));

		for (j = i + 1; j < NITEMS; j++) {
			expect_true(ckh_search(&ckh, p[j], NULL, NULL), "Unexpected ckh_search() failure");
			expect_true(ckh_remove(&ckh, p[j], &q, &r), "Unexpected ckh_remove() failure");
			expect_ptr_eq(p[j], q, "Key pointer mismatch");
			expect_ptr_eq(p[j], r, "Value pointer mismatch");
			expect_false(ckh_search(&ckh, p[j], NULL, NULL), "Unexpected ckh_search() success");
			expect_false(ckh_remove(&ckh, p[j], &q, &r), "Unexpected ckh_remove() success");
		}

		{
			bool seen[NITEMS] = { 0 };

			for (size_t tabind = 0; ckh_iter(&ckh, &tabind, &q, &r);) {
				expect_ptr_eq(q, r, "Key and val not equal");

				for (size_t k = 0; k < NITEMS; k++) {
					if (p[k] == q) {
						expect_false(seen[k], "Item %zu already seen", k);
						seen[k] = true;
						break;
					}
				}
			}

			for (j = 0; j < i + 1; j++) {
				expect_true(seen[j], "Item %zu not seen", j);
			}
			for (; j < NITEMS; j++) {
				expect_false(seen[j], "Item %zu seen", j);
			}
		}
	}

	for (i = 0; i < NITEMS; i++) {
		expect_true(ckh_search(&ckh, p[i], NULL, NULL), "Unexpected ckh_search() failure");
		expect_true(ckh_remove(&ckh, p[i], &q, &r), "Unexpected ckh_remove() failure");
		expect_ptr_eq(p[i], q, "Key pointer mismatch");
		expect_ptr_eq(p[i], r, "Value pointer mismatch");
		expect_false(ckh_search(&ckh, p[i], NULL, NULL), "Unexpected ckh_search() success");
		expect_false(ckh_remove(&ckh, p[i], &q, &r), "Unexpected ckh_remove() success");
		free(p[i]);
	}

	expect_zu_eq(ckh_count(&ckh), 0, "ckh_count() should return %zu, but it returned %zu", SZ(0), ckh_count(&ckh));
	ckh_delete(&ckh);
#undef NITEMS
}
TEST_END

int main(void)
{
	return test(test_new_delete, test_count_insert_search_remove, test_insert_iter_remove);
}
