#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <time.h>

#include "../common_tools.h"

#define FILE_LENGTH 	0x1000

int mm_prepare_mapped_mem(char *file);
void mm_append_to_XMLfile(int runtime_count,char *content, char *file_memory);
int mm_get_next_available(char *file_memory, int size);


#endif //MEMORY_MAP_H_
