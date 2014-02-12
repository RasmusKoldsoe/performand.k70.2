#include <stdio.h>
#include <string.h>

#define MAXSIZE 4032
#define MAXTOKEN 20

const char *line = "10 '\n' 0 \"<SENSOR1 timestamp=%s>\n%s\n<\\SENSOR1>\" \"<var1>%s<\\var1>\" \"<var2>%s<\\var2>\" \"<var3>%s<\\var3>\" \"<var4>%s<\\var4>\" \"<var5>%s<\\var5>\" \"<var6>%s<\\var6>\" \"<var7>%s<\\var7>\" \"<var8>%s<\\var8>\" \"<var9>%s<\\var9>\"";
const char *data = "123.456,1,,3,4,,6,7,8,9";


int main(void)
{
	char delimitor;
	char s[MAXTOKEN][MAXSIZE], d[MAXTOKEN][MAXSIZE];
	int token_count, show_unused;

	int count = sscanf(line, "%d " "'%c' " "%d ", &token_count, &delimitor, &show_unused);
	int rcount = 4 + 2 + 2 +(token_count/10);
	int orig_len = strlen(line);

	for(count=0; count<token_count && rcount < orig_len; count++) {
		sscanf(line+rcount, "\"%[^\"]\"",  s[count]);
		rcount += strlen(s[count]) + 3;
//printf("line token: %s\n", s[count]);
	}

	for(count=0, rcount = 0; count<token_count; count++) {
		sscanf(data+rcount, "%[^,],", d[count]);
		rcount += strlen(d[count]) + 1;
//printf("data token: %s\n", d[count]);
	}

	char f_data[300];
	int f_data_len = 0;
	memset(f_data, '\0', sizeof(f_data));

	for(count=1; count<token_count; count++) {
		if( show_unused == 0 && strlen(d[count]) == 0 )
			continue;
//printf("data token: %s\tline token: %s\n", d[count], s[count]);
		f_data_len += sprintf(f_data + f_data_len, s[count], d[count] );
		f_data_len += sprintf(f_data + f_data_len, "%c", count==token_count-1?'\0':delimitor);
	}

	printf(s[0], d[0], f_data);
	printf("\n");

	return 0;
}
