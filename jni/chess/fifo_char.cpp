#include "FifoQueue.h"

extern "C" {
	#include "fifo_char.h"
}

FifoCharErrorHandlerFn _handler;

void fifo_char_set_error_handler(FifoCharErrorHandlerFn fn) {
	_handler = fn;
}

int fifo_char_create(void **ppv, size_t size) {

	*ppv = reinterpret_cast<charfifo *>(new FifoQueue<char>(size));
	return 0;
}

void fifo_char_destroy(void **ppv) {

	delete reinterpret_cast<FifoQueue<char> *>(*ppv);
	*ppv = NULL;
}

char fifo_char_put(void **ppv, const char ch) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->put(ch);
}

char fifo_char_get(void **ppv) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->get();
}

size_t fifo_char_size(void ** ppv) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->size();
}

size_t fifo_char_capacity(void ** ppv) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->capacity();
}

int fifo_char_full(void ** ppv) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->full();
}

int fifo_char_empty(void ** ppv) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->empty();
}

void fifo_char_clear(void **ppv) {
	reinterpret_cast<FifoQueue<char> *>(*ppv)->clear();
}

int fifo_char_read(void **ppv, char * buf, size_t count) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->read(buf, count);
}

int fifo_char_write(void **ppv, char * buf, size_t count) {
	return reinterpret_cast<FifoQueue<char> *>(*ppv)->write(buf, count);
}
