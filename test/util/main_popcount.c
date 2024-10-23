#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "bit_map.h"
#include "../test.h"

//Brian Kernighan 算法
int popcount_(unsigned long long x)
{
	unsigned int count = 0;

	while (x != 0) {
		x &= x - 1;
		count++;
	}

	return count;
}

#define NUM 10000

TEST_BEGIN(popcount)
{
	unsigned long long test[NUM];
	int err = 0;

	srand(time(NULL));
	for (int i = 0; i < NUM; i++)
		test[i] = ((unsigned long long)rand()) << rand();

	for (int i = 0; i < 10; i++)
		printf("%llx, %d\n", test[i], popcount_u64(test[i]));
	printf("\n");

	for (int i = 0; i < NUM; i++)
		if (popcount_u64(test[i]) != popcount_(test[i])) {
			printf("fail: %llx\n", test[i]);
			err++;
		}

	if (!err)
		printf("success!\n");
}
TEST_END

int main(void)
{
	return test(popcount);
}