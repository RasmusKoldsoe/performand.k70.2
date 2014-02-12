#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


void _setDate(const char* dataStr)  // format like MMDDYY
{
  char buf[3] = {0};

  strncpy(buf, dataStr + 0, 2);
  unsigned short month = atoi(buf);

  strncpy(buf, dataStr + 2, 2);
  unsigned short day = atoi(buf);

  strncpy(buf, dataStr + 4, 2);
  unsigned short year = atoi(buf);

  time_t mytime = time(0);
  struct tm* tm_ptr = localtime(&mytime);

  if (tm_ptr)
  {
    tm_ptr->tm_mon  = month - 1;
    tm_ptr->tm_mday = day;
    tm_ptr->tm_year = year + (2000 - 1900);

    const struct timeval tv = {mktime(tm_ptr), 0};
    settimeofday(&tv, 0);
  }
}

int main(int argc, char** argv)
{
  if (argc < 1)
  {
    printf("enter a date using the format MMDDYY\n");
    return 1;
  }

  _setDate(argv[1]);

  return 0;
}
