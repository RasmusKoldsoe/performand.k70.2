#include "memory_map.h"
#include <fcntl.h>

struct timespec spec;
long ms, s;

int mm_get_next_available(h_mmapped_file *mapped_file, int size_required)
{
	int mem_ptr = 0;
	//struct timespec spec;
	long s, ms;

	mem_ptr = *mapped_file->mem_ptr;
	mem_ptr = (mem_ptr<<8) + *(mapped_file->mem_ptr+1);

	debug(3, "[%s daemon] File size: 0x%X\n", mapped_file->filename,mem_ptr);

	if((mem_ptr+size_required+2) > mapped_file->size) {

		//clock_gettime(CLOCK_REALTIME, &spec);
		
		debug(2, "[%s daemon] Reset buffer because 0x%X > 0x%X\n", 
					mapped_file->filename, 
					mem_ptr+size_required+2, 
					mapped_file->size);

		memset(mapped_file->mem_ptr,0, mapped_file->size);
		mem_ptr=0;	
	}

	return mem_ptr;
}

void mm_append(char *content, h_mmapped_file *mapped_file) {
	int len,fd = 0;	
	int mem_ptr = 0;

	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	
	len=strlen(content);
	
	//Locking !!!
    	fl.l_pid = getpid();
	fd = open(mapped_file->filename, O_WRONLY);  	// get the file descriptor	

	fcntl(fd, F_SETLKW, &fl);  			// set the lock, waiting if necessary
	
	/* ****** CRITICAL REGION *********** */
	mem_ptr = mm_get_next_available(mapped_file, len);
	
	sprintf((char*) mapped_file->mem_ptr+2+mem_ptr, "%s",content);

	*mapped_file->mem_ptr = (char)(((len+mem_ptr) & 0xFF00)>>8);
	*(mapped_file->mem_ptr+1) = (char)((len+mem_ptr) & 0xFF);

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
*/
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
