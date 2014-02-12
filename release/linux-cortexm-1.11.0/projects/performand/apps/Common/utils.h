#ifndef _UTILS_H_
#define _UTILS_H_

#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC	1000000000L
#define NSEC_PER_MSEC   1000000L
#define NSEC_PER_USEC   1000L

#define LOCK_FILE(fl, file_name, fd, o_flags) { fl.l_pid = getpid(); \
					  			   				fd = open(file_name, o_flags); \
					  			   				if(fd < 0){ fprintf(stderr, "Can not open file %s\n", file_name); return; } \
					  			   				fcntl(fd, F_SETLKW, &fl); }

#define UNLOCK_FILE(fd) { fl.l_type  = F_UNLCK; \
						  fcntl(fd, F_SETLK, &fl); \
						  close(fd); }

#define GET_FILE_SIZE(file_size, fd) { file_size = lseek(fd, 0, SEEK_END); \
								       lseek(fd, 0, SEEK_SET); }

extern int rw_rnt_count(void);
extern int read_rt_count(void);
extern int write_log_file(char *file,int runtime_count, int file_idx, char *data);
extern void print_char_array(char *buff, int length, int offset);
extern void print_byte_array(char *buff, int length, int offset);
extern int setDate(int dd, int mm, int yy, int h, int min, int sec);

extern void set_normalized_timespec(struct timespec *ts, time_t sec, signed long nsec);
extern struct timespec subtract_timespec(struct timespec lhs, struct timespec rhs);
extern void format_timespec(char* str, struct timespec *ts);

#endif
