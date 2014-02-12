#include <stdio.h>
#include <stdlib.h>

int main (void)
{
	FILE *pp;
	pp = popen("ls -al /sensors", "r");
	if (pp != NULL) {
		while(1) {
			char *line;
			char buff[1000];
			line = fgets(buff, sizeof(buff), pp);
			if (line == NULL) break;
			line[strlen(line)-1] = '\0';
			if (line[0] == '-')	printf("%d %s\n", strlen(line+57), line+57);
		}
	}
	pclose(pp);
	return 0;
}

