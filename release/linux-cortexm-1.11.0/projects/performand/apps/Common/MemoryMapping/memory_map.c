#include "memory_map.h"
#include <fcntl.h>

#define MM_FILE_INDEX_SIZE         2


int get_mem_index(h_mmapped_file *mapped_file) 
{
	int mem_index = *mapped_file->mem_ptr; 
	return (mem_index<<8) + *(mapped_file->mem_ptr+1);
}

void set_mem_index(h_mmapped_file *mapped_file, int index)
{
	*mapped_file->mem_ptr = (char)(( index & 0xFF00 ) >> 8 );
	*(mapped_file->mem_ptr+1) = (char)( index & 0xFF );
}


int mm_get_next_available(h_mmapped_file *mapped_file, int size_required)
{
	int mem_index_ptr = get_mem_index(mapped_file);

	debug(3, "[%s daemon] File size: 0x%X\n", mapped_file->filename, mem_index_ptr);

	if((mem_index_ptr + size_required + MM_FILE_INDEX_SIZE) > mapped_file->size) {
		debug(2, "[%s daemon] Reset buffer because 0x%X > 0x%X\n", 
					mapped_file->filename, 
					mem_index_ptr + size_required + MM_FILE_INDEX_SIZE, 
					mapped_file->size);

		memset(mapped_file->mem_ptr, 0, mapped_file->size);
		mem_index_ptr = 0;	
	}

	return mem_index_ptr;
}

void mm_append(char *content, h_mmapped_file *mapped_file)
{
	int fd = 0;	
	int mem_index_ptr = 0;
	int len = strlen(content);
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	
	//Locking !!!
	fl.l_pid = getpid();
	fd = open(mapped_file->filename, O_WRONLY);	// get the file descriptor	
	fcntl(fd, F_SETLKW, &fl);					// set the lock, waiting if necessary	
	
	/* ****** CRITICAL REGION *********** */
	mem_index_ptr = mm_get_next_available(mapped_file, len);
	
	sprintf((char*) mapped_file->mem_ptr + MM_FILE_INDEX_SIZE + mem_index_ptr, "%s", content);

	set_mem_index(mapped_file, mem_index_ptr + len);

	// TODO: A problem occurs here since the write may overlap the lock so the read will 
	// quite possibly issued before the actual write is finished ?! Still to be confirmed!
	// As a first solution I placed a memory barrier, DSB. See Cortex-M manual.
	// DSB acts as a special data synchronization memory barrier. Instructions that come after the DSB, 
	// in program order, do not execute until the DSB instruction completes. The DSB instruction 
	// completes when all explicit memory accesses before it complete.
	// For safety I added a delay ...
	asm("dsb");
	usleep(5000);

	/* ****** END CRITICAL REGION ******* */

	//Unlocking !!!
	fl.l_type   = F_UNLCK;  			// tell it to unlock the region
	fcntl(fd, F_SETLK, &fl); 			// set the region to unlocked
	close(fd);
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

	mapped_file->mem_ptr = (char*)mmap (0, mapped_file->size, PROT_WRITE, MAP_SHARED, fd, 0);
	if(mapped_file->mem_ptr == MAP_FAILED) {
    	perror("ERROR: mm_prepare_mapped_mem mmap");
		return -1;
    }

	memset(mapped_file->mem_ptr, 0, mapped_file->size);

	debug(1, "[%s daemon] Buffer memory mapped at %02X [%d Bytes]\n",mapped_file->filename, mapped_file->mem_ptr, mapped_file->size);

	close(fd);
	
	return 1;		
}
