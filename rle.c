#include <stdio.h>
#include <stdlib.h>
#include "rle.h"

int rle_open(const char *path, RbwtFile *file) {
    FILE *fp;
    long size;

    if (path == NULL || file == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    file->fp = fp;
    file->num_bytes = size;
    file->total_len = 0; 

    return 0;
}

void rle_close(RbwtFile *file) {
    if (file == NULL) {
        return;
    }
    if (file->fp != NULL) {
        fclose(file->fp);
        file->fp = NULL;
    }
}

int rle_read_byte(RbwtFile *file, long offset, unsigned char *out) {
    if (file == NULL || file->fp == NULL || offset < 0 || offset >= file->num_bytes) {
        return -1;
    }

    if (fseek(file->fp, offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(out, 1, 1, file->fp) != 1) {
        return -1;
    }
    return 0;
}

long rle_read_chunk(RbwtFile *file, long offset, unsigned char *buf, long max_count) {
    long remaining;
    size_t to_read;
    size_t got;

    if (file == NULL || file->fp == NULL || buf == NULL || offset < 0 || max_count <= 0) {
        return -1;
    }
    if (offset >= file->num_bytes) {
        return 0;
    }

    if (fseek(file->fp, offset, SEEK_SET) != 0) {
        return -1;
    }

    remaining = file->num_bytes - offset;
    to_read = (size_t) (remaining < max_count ? remaining : max_count);
    got = fread(buf, 1, to_read, file->fp);
    return (long) got;
}

void rle_cursor_init(RleCursor *cur, RbwtFile *file, long start_run) {
    cur->file = file;
    cur->run_index = start_run;
    cur->buf_start = 0;
    cur->buf_len = 0;
}

int rle_cursor_next(RleCursor *cur, unsigned char *out) {
    if (cur->run_index < cur->buf_start || cur->run_index >= cur->buf_start + cur->buf_len) {
        long got = rle_read_chunk(cur->file, cur->run_index, cur->buf, RLE_CURSOR_BUFSZ);
        if (got <= 0) {
            return -1;
        }
        cur->buf_start = cur->run_index;
        cur->buf_len = got;
    }

    *out = cur->buf[cur->run_index - cur->buf_start];
    cur->run_index++;
    return 0;
}

void rle_decode_byte(unsigned char b, unsigned char *symbol, unsigned char *length) {
    *symbol = (b >> 5) & 0x07;
    *length = (b & 0x1F) + 1;
}

char rle_symbol_to_char(unsigned char symbol) {
    switch (symbol) {
        case SYM_A:  return 'A';
        case SYM_C:  return 'C';
        case SYM_G:  return 'G';
        case SYM_T:  return 'T';
        case SYM_NL: return '\n';
        default:     return '?';
    }
}

int rle_char_to_symbol(char c) {
    switch (c) {
        case 'A':  return SYM_A;
        case 'C':  return SYM_C;
        case 'G':  return SYM_G;
        case 'T':  return SYM_T;
        case '\n': return SYM_NL;
        default:   return -1;
    }
}
