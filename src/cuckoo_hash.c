/*
 *******************************************************************************
 * Implementation of (2^1+,2) cuckoo hashing, where 2^1+ indicates that each
 * hash bucket contains 2^n cells, for n >= 1, and 2 indicates that two hash
 * functions are employed.  The original cuckoo hashing algorithm was described
 * in:
 *
 *   Pagh, R., F.F. Rodler (2004) Cuckoo Hashing.  Journal of Algorithms
 *     51(2):122-144.
 *
 * Generalization of cuckoo hashing was discussed in:
 *
 *   Erlingsson, U., M. Manasse, F. McSherry (2006) A cool and practical
 *     alternative to traditional hash tables.  In Proceedings of the 7th
 *     Workshop on Distributed Data and Structures (WDAS'06), Santa Clara, CA,
 *     January 2006.
 *
 * This implementation uses precisely two hash functions because that is the
 * fewest that can work, and supporting multiple hashes is an implementation
 * burden.  Here is a reproduction of Figure 1 from Erlingsson et al. (2006)
 * that shows approximate expected maximum load factors for various
 * configurations:
 *
 *           |         #cells/bucket         |
 *   #hashes |   1   |   2   |   4   |   8   |
 *   --------+-------+-------+-------+-------+
 *         1 | 0.006 | 0.006 | 0.03  | 0.12  |
 *         2 | 0.49  | 0.86  |>0.93< |>0.96< |
 *         3 | 0.91  | 0.97  | 0.98  | 0.999 |
 *         4 | 0.97  | 0.99  | 0.999 |       |
 *
 * The number of cells per bucket is chosen such that a bucket fits in one cache
 * line.  So, on 32- and 64-bit systems, we use (8,2) and (4,2) cuckoo hashing,
 * respectively.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cuckoo_hash.h"
#include "prng.h"
#include "hash.h"

static bool ckh_grow(ckh_s *ckh);
static void ckh_shrink(ckh_s *ckh);

//Search bucket for key and return the cell number if found; CKH_NONE otherwise.
static size_t ckh_bucket_search(ckh_s *ckh, size_t bucket, const void *key)
{
	ckh_cell_s *cell;
	size_t i, pos;

	for (i = 0; i < (SZ(1) << LG_CKH_BUCKET_CELLS); i++) {
		pos = (bucket << LG_CKH_BUCKET_CELLS) + i;
		cell = &ckh->tab[pos];

		if (cell->key && ckh->keycomp(key, cell->key))
			return pos;
	}

	return CKH_NONE;
}

//Search table for key and return cell number if found; CKH_NONE otherwise.
static size_t ckh_isearch(ckh_s *ckh, const void *key)
{
	size_t hashes[2], bucket, cell;

	assert(ckh != NULL);

	ckh->hash(key, hashes);

	//Search primary bucket.
	bucket = hashes[0] & ((SZ(1) << ckh->lg_curbuckets) - 1);
	cell = ckh_bucket_search(ckh, bucket, key);
	if (cell != CKH_NONE)
		return cell;

	//Search secondary bucket.
	bucket = hashes[1] & ((SZ(1) << ckh->lg_curbuckets) - 1);
	cell = ckh_bucket_search(ckh, bucket, key);
	return cell;
}

static bool ckh_try_bucket_insert(ckh_s *ckh, size_t bucket, const void *key, const void *data)
{
	ckh_cell_s *cell;
	unsigned offset, i;

	//Cycle through the cells in the bucket, starting at a random position.
	//The randomness avoids worst-case search overhead as buckets fill up.
	offset = (unsigned)prng_lg_range_u64(&ckh->prng_state, LG_CKH_BUCKET_CELLS);
	for (i = 0; i < (SZ(1) << LG_CKH_BUCKET_CELLS); i++) {
		cell = &ckh->tab[(bucket << LG_CKH_BUCKET_CELLS) + ((i + offset) & ((SZ(1) << LG_CKH_BUCKET_CELLS) - 1))];
		if (!cell->key) {
			cell->key = key;
			cell->data = data;
			ckh->count++;
			return true;
		}
	}

	return false;
}

//No space is available in bucket.  Randomly evict an item, then try to find an alternate location for that item.
//Iteratively repeat this eviction/relocation procedure until either success or detection of an
//eviction/relocation bucket cycle.
static bool ckh_evict_reloc_insert(ckh_s *ckh, size_t argbucket, void const **argkey, void const **argdata)
{
	const void *key, *data, *tkey, *tdata;
	ckh_cell_s *cell;
	size_t hashes[2], bucket, tbucket;
	unsigned i;

	bucket = argbucket;
	key = *argkey;
	data = *argdata;
	while (true) {
#ifdef CKH_DEBUG
		ckh->nrelocs++;
#endif
		i = (unsigned)prng_lg_range_u64(&ckh->prng_state, LG_CKH_BUCKET_CELLS);
		cell = &ckh->tab[(bucket << LG_CKH_BUCKET_CELLS) + i];
		assert(cell->key != NULL);

		/* Swap cell->{key,data} and {key,data} (evict). */
		tkey = cell->key;
		tdata = cell->data;
		cell->key = key;
		cell->data = data;
		key = tkey;
		data = tdata;

		/* Find the alternate bucket for the evicted item. */
		ckh->hash(key, hashes);
		tbucket = hashes[1] & ((SZ(1) << ckh->lg_curbuckets) - 1);
		if (tbucket == bucket) {
			tbucket = hashes[0] & ((SZ(1) << ckh->lg_curbuckets) - 1);
		}
		/* Check for a cycle. */
		if (tbucket == argbucket) {
			*argkey = key;
			*argdata = data;
			return false;
		}

		bucket = tbucket;
		if (ckh_try_bucket_insert(ckh, bucket, key, data)) {
			return true;
		}
	}
}

static bool ckh_try_insert(ckh_s *ckh, void const **argkey, void const **argdata)
{
	size_t hashes[2], bucket;
	const void *key = *argkey;
	const void *data = *argdata;

	ckh->hash(key, hashes);

	//Try to insert in primary bucket.
	bucket = hashes[0] & ((SZ(1) << ckh->lg_curbuckets) - 1);
	if (ckh_try_bucket_insert(ckh, bucket, key, data))
		return true;

	//Try to insert in secondary bucket.
	bucket = hashes[1] & ((SZ(1) << ckh->lg_curbuckets) - 1);
	if (ckh_try_bucket_insert(ckh, bucket, key, data))
		return true;

	//Try to find a place for this item via iterative eviction/relocation.
	return ckh_evict_reloc_insert(ckh, bucket, argkey, argdata);
}

//Try to rebuild the hash table from scratch by inserting all items from the old table into the new.
static bool ckh_rebuild(ckh_s *ckh, ckh_cell_s *aTab)
{
	size_t count, i, nins;
	const void *key, *data;

	count = ckh->count;
	ckh->count = 0;
	for (i = nins = 0; nins < count; i++) {
		if (aTab[i].key) {
			key = aTab[i].key;
			data = aTab[i].data;
			if (!ckh_try_insert(ckh, &key, &data)) { //success will ++ckh->count
				ckh->count = count;
				return false;
			}
			nins++;
		}
	}

	return true;
}

static bool ckh_grow(ckh_s *ckh)
{
	ckh_cell_s *tab, *newtab;
	unsigned lg_prevbuckets, lg_curcells;
	size_t usize;

	lg_prevbuckets = ckh->lg_curbuckets;
	lg_curcells = ckh->lg_curbuckets + LG_CKH_BUCKET_CELLS;
	tab = ckh->tab;

	while (true) {
		lg_curcells++;
		usize = ALIGNMENT_CEILING(sizeof(ckh_cell_s) << lg_curcells, CACHELINE);
		newtab = (ckh_cell_s *)ckh->malloc(usize, ckh->data);
		if (!newtab) {
			ckh->tab = tab;
			ckh->lg_curbuckets = lg_prevbuckets;
			return false;
		}

		//Swap in new table.
		ckh->tab = newtab;
		ckh->lg_curbuckets = lg_curcells - LG_CKH_BUCKET_CELLS;
		if (ckh_rebuild(ckh, tab)) { //rebuild success
			ckh->free(tab);
#ifdef CKH_DEBUG
			ckh->ngrows++;
#endif
			return true;
		}

		ckh->free(newtab); //Rebuilding failed, so back out partially rebuilt table.
	}
}

static void ckh_shrink(ckh_s *ckh)
{
	ckh_cell_s *tab, *newtab;
	size_t usize;
	unsigned lg_prevbuckets, lg_curcells;

	lg_prevbuckets = ckh->lg_curbuckets;
	lg_curcells = ckh->lg_curbuckets + LG_CKH_BUCKET_CELLS - 1;
	tab = ckh->tab;

	usize = ALIGNMENT_CEILING(sizeof(ckh_cell_s) << lg_curcells, CACHELINE);
	newtab = (ckh_cell_s *)ckh->malloc(usize, ckh->data);
	if (!newtab)
		return;

	//Swap in new table.
	ckh->tab = newtab;
	ckh->lg_curbuckets = lg_curcells - LG_CKH_BUCKET_CELLS;

	if (ckh_rebuild(ckh, tab)) {
		ckh->free(tab);
#ifdef CKH_DEBUG
		ckh->nshrinks++;
#endif
	} else {
		//Rebuilding failed, so back out partially rebuilt table.
		ckh->free(newtab);
		ckh->tab = tab;
		ckh->lg_curbuckets = lg_prevbuckets;
#ifdef CKH_DEBUG
		ckh->nshrinkfails++;
#endif
	}

	return;
}

size_t ckh_count(ckh_s *ckh)
{
	assert(ckh != NULL);

	return ckh->count;
}

bool ckh_iter(ckh_s *ckh, size_t *tabiter, void **key, void **data)
{
	size_t i, ncells;

	for (i = *tabiter, ncells = (SZ(1) << (ckh->lg_curbuckets + LG_CKH_BUCKET_CELLS)); i < ncells; i++) {
		if (ckh->tab[i].key) {
			if (key)
				*key = (void *)ckh->tab[i].key;
			if (data)
				*data = (void *)ckh->tab[i].data;

			*tabiter = i + 1;
			return true;
		}
	}

	return false;
}

bool ckh_insert(ckh_s *ckh, const void *key, const void *data)
{
	assert(ckh != NULL);
	assert(!ckh_search(ckh, key, NULL, NULL));

#ifdef CKH_DEBUG
	ckh->ninserts++;
#endif

	while (!ckh_try_insert(ckh, &key, &data)) {
		if (!ckh_grow(ckh)) {
			return false;
		}
	}

	return true;
}

bool ckh_remove(ckh_s *ckh, const void *searchkey, void **key, void **data)
{
	size_t cell;

	assert(ckh != NULL);

	cell = ckh_isearch(ckh, searchkey);
	if (cell != CKH_NONE) {
		if (key)
			*key = (void *)ckh->tab[cell].key;
		if (data)
			*data = (void *)ckh->tab[cell].data;
		ckh->tab[cell].key = NULL;
		ckh->tab[cell].data = NULL; /* Not necessary. */

		ckh->count--;
		//Try to halve the table if it is less than 1/4 full.
		if (ckh->count < (SZ(1) << (ckh->lg_curbuckets + LG_CKH_BUCKET_CELLS - 2)) && ckh->lg_curbuckets > ckh->lg_minbuckets)
			ckh_shrink(ckh);

		return true;
	}

	return false;
}

bool ckh_search(ckh_s *ckh, const void *searchkey, void **key, void **data)
{
	size_t cell;

	assert(ckh != NULL);

	cell = ckh_isearch(ckh, searchkey);
	if (cell != CKH_NONE) {
		if (key)
			*key = (void *)ckh->tab[cell].key;
		if (data)
			*data = (void *)ckh->tab[cell].data;
		return true;
	}

	return false;
}

bool ckh_new(ckh_s *ckh, size_t minitems, ckh_hash_f *ckh_hash, ckh_keycomp_f *keycomp, ckh_malloc_f *ckh_malloc, ckh_free_f *ckh_free, void *data)
{
	size_t mincells, usize;
	volatile unsigned lg_mincells;

	assert(minitems > 0);
	assert(ckh_hash);
	assert(keycomp);
	assert(ckh_malloc);
	assert(ckh_free);

#ifdef CKH_DEBUG
	ckh->ngrows = 0;
	ckh->nshrinks = 0;
	ckh->nshrinkfails = 0;
	ckh->ninserts = 0;
	ckh->nrelocs = 0;
#endif
	ckh->prng_state = 42; /* Value doesn't really matter. */
	ckh->count = 0;

	//Find the minimum power of 2 that is large enough to fit minitems entries.
	//We are using (2+,2) cuckoo hashing, which has an expected maximum load factor of at least ~0.86,
	//so 0.75 is a conservative load factor that will typically allow mincells items to fit without ever growing the table.
	assert(LG_CKH_BUCKET_CELLS > 0);
	mincells = ((minitems + (3 - (minitems % 3))) / 3) << 2;
	for (lg_mincells = LG_CKH_BUCKET_CELLS; (SZ(1) << lg_mincells) < mincells; lg_mincells++) {
		//Do nothing.
	}
	ckh->lg_minbuckets = lg_mincells - LG_CKH_BUCKET_CELLS;
	ckh->lg_curbuckets = lg_mincells - LG_CKH_BUCKET_CELLS;
	ckh->hash = ckh_hash;
	ckh->keycomp = keycomp;
	ckh->malloc = ckh_malloc;
	ckh->free = ckh_free;
	ckh->data = data;

	usize = ALIGNMENT_CEILING(sizeof(ckh_cell_s) << lg_mincells, CACHELINE);
	ckh->tab = (ckh_cell_s *)ckh->malloc(usize, ckh->data);
	if (!ckh->tab)
		return false;

	return true;
}

void ckh_delete(ckh_s *ckh)
{
	assert(ckh != NULL);

#ifdef CKH_DEBUG
	printf("%s(%p): ngrows: %lld, nshrinks: %lld, nshrinkfails: %lld, ninserts: %lld, nrelocs: %lld\n", __func__, ckh,
	       (unsigned long long)ckh->ngrows, (unsigned long long)ckh->nshrinks, (unsigned long long)ckh->nshrinkfails,
	       (unsigned long long)ckh->ninserts, (unsigned long long)ckh->nrelocs);
#endif

	ckh->free(ckh->tab);

	return;
}

//help function
void ckh_string_hash(const void *key, size_t r_hash[2])
{
	cuckoo_hash(key, strlen((const char *)key), 0x94122f33U, r_hash);
}

bool ckh_string_keycomp(const void *k1, const void *k2)
{
	assert(k1 != NULL);
	assert(k2 != NULL);

	return !strcmp((char *)k1, (char *)k2);
}

void ckh_pointer_hash(const void *key, size_t r_hash[2])
{
	union {
		const void *v;
		size_t i;
	} u;

	assert(sizeof(u.v) == sizeof(u.i));
	u.v = key;
	cuckoo_hash(&u.i, sizeof(u.i), 0xd983396eU, r_hash);
}

bool ckh_pointer_keycomp(const void *k1, const void *k2)
{
	return (k1 == k2);
}

void *ckh_malloc_clean(const size_t nsize, const void *data)
{
	(void)data;
	return calloc(nsize, 1);
}

void ckh_free(void *ptr)
{
	free(ptr);
}
