#ifndef _UTILS_H_
#define _UTILS_H_

int rw_rnt_count(void);
int read_rt_count(void);
int write_log_file(char *file,int runtime_count, int file_idx, char *data);
void print_char_array(char *buff, int length, int offset);
void print_byte_array(char *buff, int length, int offset);
int setDate(int dd, int mm, int yy, int h, int min, int sec) ;

#endif