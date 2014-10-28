#ifndef ARRAY_INCLUDE
#define ARRAY_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *, size_t);
void insertArray(Array *, char);
void freeArray(Array *);
void eraseArray(Array *, int, size_t);
void copyArray(Array *, char *, size_t);

#ifdef __cplusplus
}
#endif

#endif

