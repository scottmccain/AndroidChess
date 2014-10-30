#include "lock.h"
#include "fifo_char.h"
#include "logging.h"

extern lock_t lock_buffer;

#define FIFO_SIZE 4096

static charfifo *pfifo;

int buffer_size() {
	int size;

	Lock(lock_buffer);
	size = fifo_char_size((void **)&pfifo);
	Unlock(lock_buffer);

	return size;
}

int check_buffer() {
	int retval;

	Lock(lock_buffer);
	retval = !fifo_char_empty((void **)&pfifo);
	Unlock(lock_buffer);

	return retval;
}

int read_buffer(char * pz, int size) {
	int bytes;

	Lock(lock_buffer);
	bytes = fifo_char_read((void **)&pfifo, pz, size);
	Unlock(lock_buffer);

	return bytes;
}

void initalize_buffer() {

	fifo_char_create((void **)&pfifo, FIFO_SIZE + 1);
	LOGI("Fifo queue initialized with size %d", FIFO_SIZE);
}

void buffer_write_string(char *sz) {

	Lock(lock_buffer);
	fifo_char_write((void **)&pfifo, sz, strlen(sz));
	Unlock(lock_buffer);
}

