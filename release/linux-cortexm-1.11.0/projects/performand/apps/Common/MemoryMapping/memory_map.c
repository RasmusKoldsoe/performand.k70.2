#include "memory_map.h"



int mm_get_next_available(char *file_memory, int size)
{
	int mem_ptr = 0;

	mem_ptr = *file_memory;
	mem_ptr = (mem_ptr<<8) + *(file_memory+1);

	if((mem_ptr+size+2) > FILE_LENGTH) {
		memset(file_memory,'\0', FILE_LENGTH);	
	}

	return mem_ptr;
}

void mm_append_to_XMLfile(int runtime_count, char *content, char *file_memory)
{
	int strlen = 0;	
	int mem_ptr = 0;

	mem_ptr = mm_get_next_available(file_memory, 550);
	if( mem_ptr < 0 ) {
		printf("ERROR: %d mm_get_next_available: %s", mem_ptr, strerror(errno));
		return;
	}

	strlen = sprintf((char*)(file_memory+2+mem_ptr),"\n\
<ble id=%d>\n\
  <stw>\n\
    <milliseconds>%s</milliseconds>\n\
  </stw>\n\
</ble>\n",runtime_count,content);

	*file_memory = (char)(((strlen+mem_ptr) & 0xFF00)>>8);
	*(file_memory+1) = (char)((strlen+mem_ptr) & 0xFF);
}

/* 
 * Prepare a memory mapped file
 */
int mm_prepare_mapped_mem(char *file)
{
	int fd;
	char *file_memory;

	/* Prepare a file large enough.*/
	if((fd = open (file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
		perror("ERROR: mm_prepare_mapped_mem open");
		return -1;	
	}

	/* Initialize the flock structure. */

	/* go to the location corresponding to the last byte */
	if (lseek (fd, FILE_LENGTH-1, SEEK_SET) < 0) {
		perror("ERROR: mm_prepare_mapped_mem lseek eof");
		return -1;
	}

	/* write a dummy byte at the last location */
	if (write (fd, "", 1) != 1) {
		perror("ERROR: mm_prepare_mapped_mem write");
		return -1;
	}

	//Go to the start of the file again
	if(lseek (fd, 0, SEEK_SET) < 0 ) {
		perror("ERROR: mm_prepare_mapped_mem lseek beginning of file");
		return -1;
	}

	file_memory = (char*)mmap (0, FILE_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);
	if(file_memory == MAP_FAILED)
      	{
        	perror("ERROR: mm_prepare_mapped_mem mmap");
		return -1;
        }

	memset(file_memory,0,FILE_LENGTH);

	debug(1, "%s daemon: Buffer memory mapped at %02X [%d Bytes]\n",file ,file_memory, FILE_LENGTH);

	close(fd);
	
	return (int)file_memory;		
}
