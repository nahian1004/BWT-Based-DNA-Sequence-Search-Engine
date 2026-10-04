#ifndef INDEX_H
#define INDEX_H

#include "rle.h"

typedef struct {
    long run_index;    
    long text_pos_before;    
    unsigned int count_before[NUM_SYMS];  
} Sample;

typedef struct {
    RbwtFile *file;         
    long C[NUM_SYMS];      
    long counts[NUM_SYMS]; 
    Sample *samples;     
    long num_samples;
    long sample_interval;  
} BwtIndex;

int index_build(RbwtFile *file, long sample_interval, BwtIndex *index);

void index_free(BwtIndex *index);

long index_occ(const BwtIndex *index, unsigned char symbol, long i);

long index_select(const BwtIndex *index, unsigned char symbol, long k);

unsigned char index_char_at(const BwtIndex *index, long i);

unsigned char index_first_col_symbol(const BwtIndex *index, long i);

#endif
