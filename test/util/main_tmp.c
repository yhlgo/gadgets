#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "../test.h"

int main()
{
	unsigned int i = 0xff000000;
	printf("0x%x\n", i >> 16);
}