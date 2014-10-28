#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

void initArray(Array *a, size_t initialSize) {
  a->array = (char *)malloc(initialSize * sizeof(char));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, char element) {
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (char *)realloc(a->array, a->size * sizeof(char));
  }

  if(a->array)
	  a->array[a->used++] = element;
}

void eraseArray(Array *a, int start, size_t count)
{
	int remaining = 0;
	if(a->used >= (start + count))
	{
		remaining = a->used - (start + count);
		a->used = a->used - count;

		// move remaining over
		if(remaining > 0)
			memmove(a->array + start, a->array + start + count, remaining);
	}
}

void copyArray(Array *a, char *buffer, size_t length)
{
	if(length <= a->used)
	{
		memcpy(buffer, a->array, length);
	}
}

void freeArray(Array *a) {
  free(a->array);
  a->array = 0;
  a->used = a->size = 0;
}

#ifdef __cplusplus
}
#endif
