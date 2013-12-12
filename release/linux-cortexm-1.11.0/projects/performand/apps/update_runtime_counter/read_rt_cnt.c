#include <stdio.h>
#include "../Common/utils.h"

int main (void)
{
	printf("Reading runtime counter. ");
	fflush(stdout);

	int success = read_rt_count();
	return success;
}
