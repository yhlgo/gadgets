#ifndef YU_TYPES_H
#define YU_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#ifndef nullptr
#define nullptr (void *)0
#endif

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) !!(x)
#define unlikely(x) !!(x)
#endif

#define unreachable() abort()

#define SZ(z) ((size_t)z)

#define LG_CACHELINE 6 //L1 cache line, 64 bytes
#define LG_SIZEOF_PTR 3 //pointer 64bit sys, 8 bytes
#define LG_SIZEOF_INT 2 //int, 4bytes
#define LG_SIZEOF_LONG_LONG_INT 3 //long long int, 8bytes

#define CACHELINE 64 //L1 cache line, 64 bytes
#define CACHELINE_MASK (CACHELINE - 1)

//Return the smallest cacheline multiple that is >= s.
#define CACHELINE_CEILING(s) (((s) + CACHELINE_MASK) & ~CACHELINE_MASK)

//Return the nearest aligned address at or below a.
#define ALIGNMENT_ADDR2BASE(a, alignment) ((void *)(((byte_t *)(a)) - (((uintptr_t)(a)) - ((uintptr_t)(a) & ((~(alignment)) + 1)))))

//Return the nearest aligned address at or above a.
#define ALIGNMENT_ADDR2CEILING(a, alignment) \
	((void *)(((byte_t *)(a)) + (((((uintptr_t)(a)) + (alignment - 1)) & ((~(alignment)) + 1)) - ((uintptr_t)(a)))))

//Return the offset between a and the nearest aligned address at or below a.
#define ALIGNMENT_ADDR2OFFSET(a, alignment) ((size_t)((uintptr_t)(a) & (alignment - 1)))

//Return the smallest alignment multiple that is >= s.
#define ALIGNMENT_CEILING(s, alignment) (((s) + (alignment - 1)) & ((~(alignment)) + 1))

#endif