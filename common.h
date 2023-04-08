/* This function reads and discards data from fd. It's used to ignore the HTTP
 * response header. */
void waste_data(int fd);
int write_resilient(int fd, void *buff, size_t len);
