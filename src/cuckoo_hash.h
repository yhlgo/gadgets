#ifndef CUCKOO_HASH_H
#define CUCKOO_HASH_H

#include "yu_types.h"

#define CKH_DEBUG 1

#define CKH_NONE SIZE_MAX

//There are 2^LG_CKH_BUCKET_CELLS cells in each hash table bucket. Try to fit one bucket per L1 cache line.
#define LG_CKH_BUCKET_CELLS (LG_CACHELINE - LG_SIZEOF_PTR - 1) //==64/8/2 (because ckhc_t has two pointer)

//Typedefs to allow easy function pointer passing.
typedef void ckh_hash_f(const void *, size_t[2]);
typedef bool ckh_keycomp_f(const void *, const void *);
typedef void *ckh_malloc_f(const size_t, const void *);
typedef void ckh_free_f(void *);

//Hash table cell.
typedef struct {
	const void *key;
	const void *data;
} ckh_cell_s;

//The hash table itself.
typedef struct {
#if CKH_DEBUG == 1
	//Counters used to get an idea of performance.
	uint64_t ngrows;
	uint64_t nshrinks;
	uint64_t nshrinkfails;
	uint64_t ninserts;
	uint64_t nrelocs;
#endif

	uint64_t prng_state; //Used for pseudo-random number generation.

	//Minimum and current number of hash table buckets.  There are CKH_BUCKET_CELLS cells per bucket.
	unsigned lg_minbuckets;
	unsigned lg_curbuckets;

	//Hash and comparison functions.
	ckh_hash_f *hash;
	ckh_keycomp_f *keycomp;

	//malloc/free tab memory
	ckh_malloc_f *malloc;
	ckh_free_f *free;
	void *data;

	size_t count; //Total number of items.
	ckh_cell_s *tab; //Hash table with 2^lg_curbuckets buckets.
} ckh_s;

//Lifetime management.  Minitems is the initial capacity.
bool ckh_new(ckh_s *ckh, size_t minitems, ckh_hash_f *ckh_hash, ckh_keycomp_f *keycomp, ckh_malloc_f *ckh_malloc, ckh_free_f *ckh_free, void *data);
void ckh_delete(ckh_s *ckh);

//Get the number of elements in the set.
size_t ckh_count(ckh_s *ckh);

//To iterate over the elements in the table
//tabiter is the real iter, just use:
//size_t i = 0; for (; ckh_iter(ckh, &i, &key, &data); ) {}
bool ckh_iter(ckh_s *ckh, size_t *tabiter, void **key, void **data);

//Basic hash table operations -- insert, removal, lookup.
bool ckh_insert(ckh_s *ckh, const void *key, const void *data);
bool ckh_remove(ckh_s *ckh, const void *searchkey, void **key, void **data);
bool ckh_search(ckh_s *ckh, const void *searchkey, void **key, void **data);

//Some useful hash and comparison functions for strings and pointers.
void ckh_string_hash(const void *key, size_t r_hash[2]);
bool ckh_string_keycomp(const void *k1, const void *k2);
void ckh_pointer_hash(const void *key, size_t r_hash[2]);
bool ckh_pointer_keycomp(const void *k1, const void *k2);

//user's malloc function must to clean memory, cuckoo wont clean
void *ckh_malloc_clean(const size_t nsize, const void *data);
void ckh_free(void *ptr);

#endif