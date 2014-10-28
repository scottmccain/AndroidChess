/*
 *
#ifndef FIFO_INCLUDE
#define FIFO_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
     char * buf;
     int head;
     int tail;
     int size;
} fifo_t;

void fifo_init(fifo_t * f, char * buf, int size);
int fifo_read(fifo_t * f, void * buf, int nbytes);
int fifo_write(fifo_t * f, const void * buf, int nbytes);

static int fifo_empty(fifo_t *q) {
	return (q->tail == q->head);
}

static unsigned int fifo_size(fifo_t * q) {
	return (q->head - q->tail + q->size) % q->size;
}

static unsigned int fifo_capacity(fifo_t *q) {
    return q->size - 1;
}

static int fifo_full(fifo_t *q) {
	return ((q->head + 1) % q->size) == q->tail;
}

static char fifo_put(fifo_t *q, char n) {
	q->buf[q->head] = n;
    q->head = (q->head + 1) % q->size;
    return n;
}

static char fifo_get(fifo_t *q) {

    char n = q->buf[q->tail];
    q->buf[q->tail] = 0;
    q->tail = (q->tail+ 1) % q->size;
    return n;
}


#define fifo_foreach(queue, item, task) do {			\
	while(!fifo_empty(queue)) {							\
		item = fifo_get(queue);							\
		do task while(0);								\
	}													\
}while(0);

#ifdef __cplusplus
}
#endif

#endif
*/

#include <stdlib.h>

#ifdef MSVC
#pragma once
#endif

#ifndef _FIFO_H
#define _FIFO_H

struct charfifo;
typedef struct charfifo charfifo;

typedef void(*FifoCharErrorHandlerFn)(const char * error_message);

int fifo_char_create(void **, size_t);
void fifo_char_destroy(void **);
char fifo_char_put(void **, const char);
char fifo_char_get(void **);

size_t fifo_char_size(void ** ppv);
size_t fifo_char_capacity(void ** ppv);
int fifo_char_full(void ** ppv);
int fifo_char_empty(void ** ppv);
void fifo_char_clear(void **ppv);

int fifo_char_read(void **ppv, char * buf, size_t count);
int fifo_char_write(void **ppv, char * buf, size_t count);

void fifo_char_set_error_handler(FifoCharErrorHandlerFn eh);

#endif
