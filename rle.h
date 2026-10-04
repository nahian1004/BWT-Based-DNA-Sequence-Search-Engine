#ifndef RLE_H
#define RLE_H

#include <stdio.h>

#define SYM_A    0
#define SYM_C    1
#define SYM_G    2
#define SYM_T    3
#define SYM_NL   4
#define NUM_SYMS 5

typedef struct {
    FILE *fp;
    long num_bytes;  
    long total_len;   
} RbwtFile;

int rle_open(const char *path, RbwtFile *file);

void rle_close(RbwtFile *file);

int rle_read_byte(RbwtFile *file, long offset, unsigned char *out);

long rle_read_chunk(RbwtFile *file, long offset, unsigned char *buf, long max_count);

#define RLE_CURSOR_BUFSZ 4096
typedef struct {
    RbwtFile *file;
    long run_index;                   
    unsigned char buf[RLE_CURSOR_BUFSZ];
    long buf_start;            
    long buf_len;        
} RleCursor;

void rle_cursor_init(RleCursor *cur, RbwtFile *file, long start_run);

int rle_cursor_next(RleCursor *cur, unsigned char *out);

void rle_decode_byte(unsigned char b, unsigned char *symbol, unsigned char *length);

char rle_symbol_to_char(unsigned char symbol);

int rle_char_to_symbol(char c);

#endif
