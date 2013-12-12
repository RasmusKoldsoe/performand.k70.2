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

#define DEFAULT_FILE_LENGTH 	0x1000

typedef struct {
	char *filename;
	char *mem_ptr;
	int size;
} h_mmapped_file;

int mm_prepare_mapped_mem(h_mmapped_file *mapped_file);
//void mm_append_to_XMLfile(int runtime_count,char *content, char *file_memory);
int mm_get_next_available(h_mmapped_file *mapped_file, int size_required);
void mm_append(char *content, h_mmapped_file *mapped_file);

#endif //MEMORY_MAP_H_
