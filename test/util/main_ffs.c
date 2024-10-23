#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "bit_map.h"
#include "../test.h"

int ffs2(long long int i)
{
	unsigned long long int x = i;
	int pos = 1;

	if (!i)
		return 0;
	while (!(x & 1)) {
		pos++;
		x >>= 1;
	}

	return pos;
}

int fls2(long long int i)
{
	unsigned long long int x = i;
	int pos = 0;

	while (x) {
		x >>= 1;
		pos++;
	}

	return pos;
}

#define NUM 10000

TEST_BEGIN(ffs_fls)
{
	long long int test[NUM];
	int err = 0;

	srand(time(NULL));
	for (int i = 0; i < NUM; i++)
		test[i] = ((long long int)rand()) << rand();

	for (int i = 0; i < NUM; i++)
		if (ffsll_(test[i]) != ffs2(test[i])) {
			printf("ffsll fail: %llx\n", test[i]);
			err++;
		}
	if (!err)
		printf("ffsll success!\n");

	err = 0;
	for (int i = 0; i < NUM; i++)
		if (flsll_(test[i]) != fls2(test[i])) {
			printf("flsll fail: %llx\n", test[i]);
			err++;
		}
	if (!err)
		printf("flsll success!\n");
}
TEST_END

int main(void)
{
	return test(ffs_fls);
}

// for (int i=0; i<NUM/10; i++)
//     printf("%d ", ffs1(test[i]));
// printf("\n");

// for (int i=0; i<NUM/10; i++)
//     printf("%d ", ffs2(test[i]));
// printf("\n");