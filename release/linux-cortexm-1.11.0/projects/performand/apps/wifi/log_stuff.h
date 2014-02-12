
#define LOG_FILE_MAX_SIZE 5000000


int log_init();
int log_store_data(char *data, int len);
void log_end();
