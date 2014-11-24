#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/time.h>
#include <sys/mman.h>

#include "../common_tools.h"
#include "../utils.h"
#include "memory_map.h"

union cShort {
	short s;
	char c[2];
};

unsigned int get_mem_index(h_mmapped_file *mapped_file) 
{
	union cShort cs;
	cs.c[0] = (char)*mapped_file->mem_ptr;
	cs.c[1] = (char)*(mapped_file->mem_ptr+1);

	//printf("-%d ", cs.s); fflush(stdout);

	return (unsigned int)cs.s;
}

void set_mem_index(h_mmapped_file *mapped_file, int index)
{
	union cShort cs;
	cs.s = (unsigned short)index;
	
	*mapped_file->mem_ptr = cs.c[0];
	*(mapped_file->mem_ptr+1) = cs.c[1];

	//printf("+%d ", cs.s); fflush(stdout);
}

void reset_mapped_mem(h_mmapped_file *mapped_file)
{
	//int fd = 0;	
	//struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };

	//LOCK_FILE(fl, mapped_file->filename, fd, O_RDWR);
	/* ****** CRITICAL REGION *********** */

	memset(mapped_file->mem_ptr, 0, mapped_file->size);

#if __arm__
	asm("dsb");
#endif
	usleep(5000);

	/* ****** END CRITICAL REGION ******* */
	//UNLOCK_FILE(fd);
}

int mm_get_line(h_mmapped_file *mapped_file, char* user_buffer)
{
	int fd = 0;	
	char* endOfLine;
	char* endOfText;
	unsigned short textCounter;
	unsigned int fileSize;
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };

	fileSize = mapped_file->size;
	textCounter = get_mem_index(mapped_file);
	endOfText = mapped_file->mem_ptr + MM_FILE_INDEX_SIZE + textCounter + 1;

	if(textCounter <= 0) {
		return -1;
	}

	//printf("file %s - testCounter: %d\n",mapped_file->filename, textCounter);

	if(lock_file(&fl, mapped_file->filename, &fd, O_RDWR) < 0) {
		return -2;
	}
	/* ****** CRITICAL REGION *********** */
	//printf("Mem_ptr: %p\n",mapped_file->mem_ptr);
	// Search for linefeed or \0 - Return if \0
	for (endOfLine = mapped_file->mem_ptr + MM_FILE_INDEX_SIZE; 
		(endOfLine < endOfText) && (*endOfLine != '\n') && (*endOfLine != '\0'); 
		endOfLine++);
	if(*endOfLine == '\0') {
		printf("Returning because endOfLine == '\\0' at %p (file %s begin: %p)\n", endOfLine, mapped_file->filename, mapped_file->mem_ptr + MM_FILE_INDEX_SIZE);
		//reset_mapped_mem(mapped_file);
		UNLOCK_FILE(fd);
		return -1;
	}

	// Overwrite linefeed character
	*endOfLine = '\0';

	// Copy line to user buffer - Assume enough memory
	ssize_t lineLength = (endOfLine - mapped_file->mem_ptr) - MM_FILE_INDEX_SIZE +1;

//printf("%s ", mapped_file->filename); fflush(stdout);
//printf("lineLength: "); fflush(stdout);
//printf("%d\n", lineLength); fflush(stdout);
//printf("(%s)\n", mapped_file->mem_ptr + MM_FILE_INDEX_SIZE); fflush(stdout);
//int i;
//for(i=0; i<lineLength; i++) {printf("%02X ", mapped_file->mem_ptr[i + MM_FILE_INDEX_SIZE]); fflush(stdout); }
//printf(")\n");

	if(lineLength > DEFAULT_FILE_LENGTH) {
		fprintf(stderr, "[mm_daemon] ERROR line length %d longer than file\n", lineLength);
		UNLOCK_FILE(fd);
		return -3;
	}
//	else if(lineLength == 0) { // Would happen if buffer[0]=='\n'
	else if(lineLength < 9) {
		fprintf(stderr, "[mm_daemon] ERROR Detected too short line length %d\n", lineLength);
		UNLOCK_FILE(fd);
		return 0;
	}
	else {
		memcpy(user_buffer, mapped_file->mem_ptr + MM_FILE_INDEX_SIZE, lineLength);
	}

	// Clear line from file
	textCounter -= lineLength;
	set_mem_index(mapped_file, textCounter);
	memcpy(mapped_file->mem_ptr + MM_FILE_INDEX_SIZE, ++endOfLine, textCounter);
	
	/* ****** END CRITICAL REGION ******* */
	UNLOCK_FILE(fd);

	return lineLength;
}

unsigned int mm_get_next_available(h_mmapped_file *mapped_file, unsigned int size_required)
{
	unsigned int mem_index_ptr = get_mem_index(mapped_file);

	debug(3, "[%s mmap daemon] File size: %d\n", mapped_file->filename, mem_index_ptr);

	if((mem_index_ptr + size_required + MM_FILE_INDEX_SIZE) > mapped_file->size) {
		debug(1, "[%s mmap daemon] Reset buffer because 0x%X > 0x%X\n", 
					mapped_file->filename, 
					mem_index_ptr + size_required + MM_FILE_INDEX_SIZE, 
					mapped_file->size);
			
		reset_mapped_mem(mapped_file);
		mem_index_ptr = 0;
	}

	return mem_index_ptr;
}

void mm_append(char *content, h_mmapped_file *mapped_file)
{
	int fd = 0;	
	unsigned int mem_index_ptr = 0;
	unsigned int len = strlen(content);
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };

	LOCK_FILE(fl, mapped_file->filename, fd, O_WRONLY);
	/* ****** CRITICAL REGION *********** */

	mem_index_ptr = mm_get_next_available(mapped_file, len);
//	char* start_ptr = (char*) mapped_file->mem_ptr + MM_FILE_INDEX_SIZE + mem_index_ptr;
//ssize_t slen = 0;
	sprintf((char*) mapped_file->mem_ptr + MM_FILE_INDEX_SIZE + mem_index_ptr, "%s", content);

	set_mem_index(mapped_file, mem_index_ptr + len);
//printf("APPEND: len=%d, slen=%d, maplen=%d\n", len, slen, get_mem_index( mapped_file ) );

	// TODO: A problem occurs here since the write may overlap the lock so the read will 
	// quite possibly issued before the actual write is finished ?! Still to be confirmed!
	// As a first solution I placed a memory barrier, DSB. See Cortex-M manual.
	// DSB acts as a special data synchronization memory barrier. Instructions that come after the DSB, 
	// in program order, do not execute until the DSB instruction completes. The DSB instruction 
	// completes when all explicit memory accesses before it complete.
	// For safety I added a delay ...
#if __arm__
	asm("dsb");
#endif
	usleep(5000);

	/* ****** END CRITICAL REGION ******* */
	UNLOCK_FILE(fd);
}


/* 
 * Prepare a memory mapped file
 */
int mm_prepare_mapped_mem(h_mmapped_file *mapped_file)
{
	int fd;

	/* Prepare a file large enough.*/
	if((fd = open (mapped_file->filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
		perror("ERROR: mm_prepare_mapped_mem open");
		return -1;	
	}

	/* Initialize the flock structure. */

	/* go to the location corresponding to the last byte */
	if (lseek (fd, mapped_file->size-1, SEEK_SET) < 0) {
		close(fd);
		perror("ERROR: mm_prepare_mapped_mem lseek eof");
		return -1;
	}

	/* write a dummy byte at the last location */
	if (write (fd, "\0", 1) != 1) {
		close(fd);
		perror("ERROR: mm_prepare_mapped_mem write");
		return -1;
	}

	//Go to the start of the file again
	if(lseek (fd, 0, SEEK_SET) < 0 ) {
		close(fd);
		perror("ERROR: mm_prepare_mapped_mem lseek beginning of file");
		return -1;
	}

	mapped_file->mem_ptr = (char*)mmap( 0, mapped_file->size, PROT_WRITE, MAP_SHARED, fd, 0 );
	if(mapped_file->mem_ptr == MAP_FAILED) {
		close(fd);
    	perror("ERROR: mm_prepare_mapped_mem mmap");
		return -1;
    }

	//memset(mapped_file->mem_ptr, 0, mapped_file->size);

	debug(1, "[%s daemon] Buffer memory mapped at %p [%d Bytes]\n",mapped_file->filename, mapped_file->mem_ptr, mapped_file->size);

	close(fd);
	
	return 1;		
}
