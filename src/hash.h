#ifndef YU_HASH_H
#define YU_HASH_H

#include "yu_types.h"
#include <string.h>

/*
 * The following hash function is based on MurmurHash3, placed into the public
 * domain by Austin Appleby.  See https://github.com/aappleby/smhasher for
 * details.
 */

static inline uint32_t hash_rotl_32(uint32_t x, int8_t r)
{
	return ((x << r) | (x >> (32 - r)));
}

static inline uint64_t hash_rotl_64(uint64_t x, int8_t r)
{
	return ((x << r) | (x >> (64 - r)));
}

static inline uint32_t hash_get_block_32(const uint32_t *p, int i)
{
	//Handle unaligned read.
	if (unlikely((uintptr_t)p & (sizeof(uint32_t) - 1)) != 0) {
		uint32_t ret;

		memcpy(&ret, (uint8_t *)(p + i), sizeof(uint32_t));
		return ret;
	}

	return p[i];
}

static inline uint64_t hash_get_block_64(const uint64_t *p, int i)
{
	//Handle unaligned read.
	if (unlikely((uintptr_t)p & (sizeof(uint64_t) - 1)) != 0) {
		uint64_t ret;

		memcpy(&ret, (uint8_t *)(p + i), sizeof(uint64_t));
		return ret;
	}

	return p[i];
}

static inline uint32_t hash_fmix_32(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

static inline uint64_t hash_fmix_64(uint64_t k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;

	return k;
}

static inline uint32_t hash_x86_32(const void *key, int len, uint32_t seed)
{
	const uint8_t *data = (const uint8_t *)key;
	const int nblocks = len / 4;
	uint32_t h1 = seed, k4 = 0, k1 = 0;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	//body
	const uint32_t *blocks = (const uint32_t *)data;
	for (int i = 0; i < nblocks; i++) {
		k4 = hash_get_block_32(blocks, i);
		k4 *= c1;
		k4 = hash_rotl_32(k1, 15);
		k4 *= c2;

		h1 ^= k4;
		h1 = hash_rotl_32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	//tail
	const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
	switch (len & 3) {
	case 3:
		k1 ^= tail[2] << 16;
	case 2:
		k1 ^= tail[1] << 8;
	case 1:
		k1 ^= tail[0];
		k1 *= c1;
		k1 = hash_rotl_32(k1, 15);
		k1 *= c2;
		h1 ^= k1;
	}

	//finalization
	h1 ^= len;
	h1 = hash_fmix_32(h1);

	return h1;
}

static inline void hash_x86_128(const void *key, const int len, uint32_t seed, uint64_t r_out[2])
{
	const uint8_t *data = (const uint8_t *)key;
	const int nblocks = len / 16;

	const uint32_t c[5] = { 0x239b961b, 0xab0e9789, 0x38b34ae5, 0xa1e38b93, 0x239b961b };
	const int8_t rotl[8] = { 15, 19, 16, 17, 17, 15, 18, 13 };
	const uint32_t s[4] = { 0x561ccd1b, 0x0bcaa747, 0x96cd1c35, 0x32ac3b17 };
	uint32_t h[4] = { seed, seed, seed, seed };
	uint32_t k4 = 0, k1 = 0;

	//body
	const uint32_t *blocks = (const uint32_t *)data;
	for (int i = 0; i < nblocks; i++) {
		for (int j = 0; j < 4; j++) {
			k4 = hash_get_block_32(blocks, i * 4 + j);
			k4 *= c[j];
			k4 = hash_rotl_32(k4, rotl[j * 2]);
			k4 *= c[j + 1];

			h[j] ^= k4;
			h[j] = hash_rotl_32(h[j], rotl[j * 2 + 1]);
			h[j] += h[(j + 1) & 3];
			h[j] = h[j] * 5 + s[j];
		}
	}

	//tail
	uint8_t tail_16[16] = { 0 };
	int tail_len = len & 15;
	const uint8_t *tail = (const uint8_t *)(data + nblocks * 16);
	memcpy(tail_16, tail, tail_len);
	for (int i = ALIGNMENT_CEILING(tail_len, 4) - 1, j = i / 4; i > 0; i -= 4, j--) {
		k1 = (uint32_t)tail_16[i] << 24 | tail_16[i - 1] << 16 | tail_16[i - 2] << 8 | tail_16[i - 3];
		k1 *= c[j];
		k1 = hash_rotl_32(k1, 18 - j);
		k1 *= c[j + 1];
		h[j] ^= k1;
	}

	//finalization
	h[0] ^= len;
	h[1] ^= len;
	h[2] ^= len;
	h[3] ^= len;

	h[0] += h[1] + h[2] + h[3];
	h[1] += h[0];
	h[2] += h[0];
	h[3] += h[0];

	h[0] = hash_fmix_32(h[0]);
	h[1] = hash_fmix_32(h[1]);
	h[2] = hash_fmix_32(h[2]);
	h[3] = hash_fmix_32(h[3]);

	h[0] += h[1] + h[2] + h[3];
	h[1] += h[0];
	h[2] += h[0];
	h[3] += h[0];

	r_out[0] = (((uint64_t)h[1]) << 32) | h[0];
	r_out[1] = (((uint64_t)h[3]) << 32) | h[2];
}

static inline void hash_x64_128(const void *key, const int len, const uint32_t seed, uint64_t r_out[2])
{
	const uint8_t *data = (const uint8_t *)key;
	const int nblocks = len / 16;

	uint64_t h1 = seed;
	uint64_t h2 = seed;

	const uint64_t c1 = 0x87c37b91114253d5ULL;
	const uint64_t c2 = 0x4cf5ad432745937fULL;

	//body
	{
		const uint64_t *blocks = (const uint64_t *)(data);

		for (int i = 0; i < nblocks; i++) {
			uint64_t k1 = hash_get_block_64(blocks, i * 2 + 0);
			uint64_t k2 = hash_get_block_64(blocks, i * 2 + 1);

			k1 *= c1;
			k1 = hash_rotl_64(k1, 31);
			k1 *= c2;

			h1 ^= k1;
			h1 = hash_rotl_64(h1, 27);
			h1 += h2;
			h1 = h1 * 5 + 0x52dce729;

			k2 *= c2;
			k2 = hash_rotl_64(k2, 33);
			k2 *= c1;

			h2 ^= k2;
			h2 = hash_rotl_64(h2, 31);
			h2 += h1;
			h2 = h2 * 5 + 0x38495ab5;
		}
	}

	//tail
	{
		const uint8_t *tail = (const uint8_t *)(data + nblocks * 16);
		uint64_t k1 = 0, k2 = 0;

		switch (len & 15) {
		case 15:
			k2 ^= ((uint64_t)(tail[14])) << 48;
		// fall through
		case 14:
			k2 ^= ((uint64_t)(tail[13])) << 40;
		// fall through
		case 13:
			k2 ^= ((uint64_t)(tail[12])) << 32;
		// fall through
		case 12:
			k2 ^= ((uint64_t)(tail[11])) << 24;
		// fall through
		case 11:
			k2 ^= ((uint64_t)(tail[10])) << 16;
		// fall through
		case 10:
			k2 ^= ((uint64_t)(tail[9])) << 8;
		// fall through
		case 9:
			k2 ^= ((uint64_t)(tail[8])) << 0;
			k2 *= c2;
			k2 = hash_rotl_64(k2, 33);
			k2 *= c1;
			h2 ^= k2;
			// fall through

		case 8:
			k1 ^= ((uint64_t)(tail[7])) << 56;
		// fall through
		case 7:
			k1 ^= ((uint64_t)(tail[6])) << 48;
		// fall through
		case 6:
			k1 ^= ((uint64_t)(tail[5])) << 40;
		// fall through
		case 5:
			k1 ^= ((uint64_t)(tail[4])) << 32;
		// fall through
		case 4:
			k1 ^= ((uint64_t)(tail[3])) << 24;
		// fall through
		case 3:
			k1 ^= ((uint64_t)(tail[2])) << 16;
		// fall through
		case 2:
			k1 ^= ((uint64_t)(tail[1])) << 8;
		// fall through
		case 1:
			k1 ^= ((uint64_t)(tail[0])) << 0;
			k1 *= c1;
			k1 = hash_rotl_64(k1, 31);
			k1 *= c2;
			h1 ^= k1;
			break;
		}
	}

	//finalization
	h1 ^= len;
	h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = hash_fmix_64(h1);
	h2 = hash_fmix_64(h2);

	h1 += h2;
	h2 += h1;

	r_out[0] = h1;
	r_out[1] = h2;
}

//API
static inline void cuckoo_hash(const void *key, size_t len, const uint32_t seed, size_t r_hash[2])
{
	assert(len <= SIZE_MAX);

#if LG_SIZEOF_PTR == 3
	hash_x64_128(key, (int)len, seed, (uint64_t *)r_hash);
#else
	uint64_t hashes[2];
	hash_x86_128(key, (int)len, seed, hashes);
	r_hash[0] = (size_t)hashes[0];
	r_hash[1] = (size_t)hashes[1];
#endif
}

#endif /* JEMALLOC_INTERNAL_HASH_H */
