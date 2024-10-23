#include <stdlib.h>

#include "../test.h"
#include "list_s.h"

/* Number of ring entries*/
#define NENTRIES 60
/* Split index, in [1..NENTRIES). */
#define SPLIT_INDEX 5

typedef struct ring_s ring_t;

struct ring_s {
	dlist(ring_t) _dl;
	char id;
};

static void init_entries(ring_t *entries)
{
	unsigned i;

	for (i = 0; i < NENTRIES; i++) {
		dl_init(&entries[i], _dl);
		entries[i].id = 'a' + i;
	}
}

static void test_independent_entries(ring_t *entries)
{
	ring_t *t;
	unsigned i, j;

	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_foreach (t, &entries[i], _dl) {
			j++;
		}
		expect_u_eq(j, 1, "Iteration over single-element ring should visit precisely one element");
	}
	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_reverse_foreach (t, &entries[i], _dl) {
			j++;
		}
		expect_u_eq(j, 1, "Iteration over single-element ring should visit precisely one element");
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_next(&entries[i], _dl);
		expect_ptr_eq(t, &entries[i], "Next element in single-element ring should be same as current element");
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_prev(&entries[i], _dl);
		expect_ptr_eq(t, &entries[i], "Previous element in single-element ring should be same as current element");
	}
}

TEST_BEGIN(test_dl_one)
{
	ring_t entries[NENTRIES];

	init_entries(entries);
	test_independent_entries(entries);
}
TEST_END

static void test_entries_ring(ring_t *entries)
{
	ring_t *t;
	unsigned i, j;

	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[(i + j) % NENTRIES].id, "Element id mismatch");
			j++;
		}
	}
	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_reverse_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[(NENTRIES + i - j - 1) % NENTRIES].id, "Element id mismatch");
			j++;
		}
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_next(&entries[i], _dl);
		expect_c_eq(t->id, entries[(i + 1) % NENTRIES].id, "Element id mismatch");
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_prev(&entries[i], _dl);
		expect_c_eq(t->id, entries[(NENTRIES + i - 1) % NENTRIES].id, "Element id mismatch");
	}
}

TEST_BEGIN(test_dl_after_insert)
{
	ring_t entries[NENTRIES];
	unsigned i;

	init_entries(entries);
	for (i = 1; i < NENTRIES; i++) {
		dl_after_insert(&entries[i - 1], &entries[i], _dl);
	}
	test_entries_ring(entries);
}
TEST_END

TEST_BEGIN(test_dl_remove)
{
	ring_t entries[NENTRIES];
	ring_t *t;
	unsigned i, j;

	init_entries(entries);
	for (i = 1; i < NENTRIES; i++) {
		dl_after_insert(&entries[i - 1], &entries[i], _dl);
	}

	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[i + j].id, "Element id mismatch");
			j++;
		}
		j = 0;
		dl_reverse_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[NENTRIES - 1 - j].id, "Element id mismatch");
			j++;
		}
		dl_remove(&entries[i], _dl);
	}
	test_independent_entries(entries);
}
TEST_END

TEST_BEGIN(test_dl_before_insert)
{
	ring_t entries[NENTRIES];
	ring_t *t;
	unsigned i, j;

	init_entries(entries);
	for (i = 1; i < NENTRIES; i++) {
		dl_before_insert(&entries[i - 1], &entries[i], _dl);
	}
	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[(NENTRIES + i - j) % NENTRIES].id, "Element id mismatch");
			j++;
		}
	}
	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_reverse_foreach (t, &entries[i], _dl) {
			expect_c_eq(t->id, entries[(i + j + 1) % NENTRIES].id, "Element id mismatch");
			j++;
		}
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_next(&entries[i], _dl);
		expect_c_eq(t->id, entries[(NENTRIES + i - 1) % NENTRIES].id, "Element id mismatch");
	}
	for (i = 0; i < NENTRIES; i++) {
		t = dl_prev(&entries[i], _dl);
		expect_c_eq(t->id, entries[(i + 1) % NENTRIES].id, "Element id mismatch");
	}
}
TEST_END

static void test_split_entries(ring_t *entries)
{
	ring_t *t;
	unsigned i, j;

	for (i = 0; i < NENTRIES; i++) {
		j = 0;
		dl_foreach (t, &entries[i], _dl) {
			if (i < SPLIT_INDEX) {
				expect_c_eq(t->id, entries[(i + j) % SPLIT_INDEX].id, "Element id mismatch");
			} else {
				expect_c_eq(t->id, entries[(i + j - SPLIT_INDEX) % (NENTRIES - SPLIT_INDEX) + SPLIT_INDEX].id, "Element id mismatch");
			}
			j++;
		}
	}
}

TEST_BEGIN(test_dl_meld_split)
{
	ring_t entries[NENTRIES];
	unsigned i;

	init_entries(entries);
	for (i = 1; i < NENTRIES; i++) {
		dl_after_insert(&entries[i - 1], &entries[i], _dl);
	}

	dl_split(&entries[0], &entries[SPLIT_INDEX], _dl);
	test_split_entries(entries);

	dl_meld(&entries[0], &entries[SPLIT_INDEX], _dl);
	test_entries_ring(entries);

	dl_meld(&entries[0], &entries[SPLIT_INDEX], _dl);
	test_split_entries(entries);

	dl_split(&entries[0], &entries[SPLIT_INDEX], _dl);
	test_entries_ring(entries);

	dl_split(&entries[0], &entries[0], _dl);
	test_entries_ring(entries);

	dl_meld(&entries[0], &entries[0], _dl);
	test_entries_ring(entries);
}
TEST_END

int main(void)
{
	return test(test_dl_one, test_dl_after_insert, test_dl_remove, test_dl_before_insert, test_dl_meld_split);
}
