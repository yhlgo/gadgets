#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "div2mul_32.h"
#include "../test.h"

#define NUM 10000

TEST_BEGIN(div_2_mul)
{
	unsigned int arr[NUM];

	srand(time(NULL));
	for (int i = 0; i < NUM; i++) {
		arr[i] = ((unsigned int)rand()) << rand();
	}

	for (int j = 0; j < NUM; j++) {
		div_info_s div_info;
		arr[j] %= 1000000;
		arr[j] = arr[j] < 2 ? 10 : arr[j];
		div_init(&div_info, arr[j]);

		for (int k = 0; k < NUM; k++) {
			int sub = div_compute(&div_info, arr[k]) - arr[k] / arr[j];
			sub = sub < 0 ? -sub : sub;
			expect_d32_le(sub, 1, "expected %u/%u: div_2_mul(%d) %d equal div %d", arr[k], arr[j], div_info.magic,
				      div_compute(&div_info, arr[k]), arr[k] / arr[j]);
		}
	}
}
TEST_END

int main(void)
{
	return test(div_2_mul);
}