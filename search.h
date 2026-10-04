#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "index.h"

typedef struct {
    long first;
    long last;  
} SearchRange;

SearchRange search_backward(const BwtIndex *index, const char *pattern, int m);

long search_LF(const BwtIndex *index, long i);

long search_FL(const BwtIndex *index, long i);

int search_extract_context(const BwtIndex *index, long row, const char *pattern,
                            int m, char *buf, size_t buf_size);

#endif
