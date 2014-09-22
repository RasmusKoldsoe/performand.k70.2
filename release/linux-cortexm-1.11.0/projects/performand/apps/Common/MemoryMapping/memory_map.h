#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

#define DEFAULT_FILE_LENGTH 	0x1000
#define MM_FILE_INDEX_SIZE         2

typedef struct {
	char *filename;
	char *mem_ptr;
	int size;
} h_mmapped_file;

//int get_mem_index(h_mmapped_file *mapped_file);
void reset_mapped_mem(h_mmapped_file *mapped_file);
int mm_get_line(h_mmapped_file *mapped_file, char* user_buffer);
int mm_prepare_mapped_mem(h_mmapped_file *mapped_file);
unsigned int mm_get_next_available(h_mmapped_file *mapped_file, unsigned int size_required);
void mm_append(char *content, h_mmapped_file *mapped_file);

#endif //MEMORY_MAP_H_
