#include <stdio.h>
#include "../Common/utils.h"

int main (void)
{
	printf("Updating runtime counter. ");
	fflush(stdout);

	int success = rw_rnt_count();
	
	if( success > 0 )
		printf("New runtime count %d\n", success);
	return success;
}
