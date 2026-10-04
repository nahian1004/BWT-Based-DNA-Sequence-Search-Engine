#include <stdio.h>
#include <stdlib.h>
#include "index.h"

static long findSampleIndex(const BwtIndex *index, long i);
static long findSampleIndexBySymbolCount(const BwtIndex *index, unsigned char symbol, long k);

int index_build(RbwtFile *file, long sample_interval, BwtIndex *index) {
    long pos;
    long counts[NUM_SYMS];
    long num_samples;
    long sample_i;
    long i;
    int s;
    unsigned char chunk[65536];

    if (file == NULL || file->fp == NULL || index == NULL || sample_interval <= 0) {
        return -1;
    }

    index->file = file;
    index->sample_interval = sample_interval;

    num_samples = (file->num_bytes / sample_interval) + 1;
    index->samples = (Sample *) malloc((size_t) num_samples * sizeof(Sample));
    if (index->samples == NULL) {
        return -1;
    }

    for (s = 0; s < NUM_SYMS; s++) {
        counts[s] = 0;
    }
    pos = 0;
    sample_i = 0;
    i = 0;

    while (i < file->num_bytes) {
        long got = rle_read_chunk(file, i, chunk, (long) sizeof(chunk));
        long ci;

        if (got <= 0) {
            free(index->samples);
            return -1;
        }

        for (ci = 0; ci < got; ci++, i++) {
            unsigned char symbol;
            unsigned char length;
            unsigned char b = chunk[ci];

            if (i % sample_interval == 0) {
                index->samples[sample_i].run_index = i;
                index->samples[sample_i].text_pos_before = pos;
                for (s = 0; s < NUM_SYMS; s++) {
                    index->samples[sample_i].count_before[s] = (unsigned int) counts[s];
                }
                sample_i++;
            }

            rle_decode_byte(b, &symbol, &length);
            counts[symbol] += length;
            pos += length;
        }
    }

    index->num_samples = sample_i;
    file->total_len = pos; 
    for (s = 0; s < NUM_SYMS; s++) {
        index->counts[s] = counts[s];
    }

    long order[NUM_SYMS] = { SYM_NL, SYM_A, SYM_C, SYM_G, SYM_T };
    long acc = 0;
    for (s = 0; s < NUM_SYMS; s++) {
        int sym = (int) order[s];
        index->C[sym] = acc;
        acc += counts[sym];
    }

    return 0;
}

void index_free(BwtIndex *index) {
    if (index == NULL) {
        return;
    }
    free(index->samples);
    index->samples = NULL;
}

long index_occ(const BwtIndex *index, unsigned char symbol, long i) {
    long sample_idx;
    long pos;
    long run;
    long count;

    if (i <= 0) {
        return 0;
    }

    sample_idx = findSampleIndex(index, i);
    pos = index->samples[sample_idx].text_pos_before;
    run = index->samples[sample_idx].run_index;
    count = (long) index->samples[sample_idx].count_before[symbol];

    {
        unsigned char b;
        RleCursor cur;
        rle_cursor_init(&cur, index->file, run);
        while (pos < i && run < index->file->num_bytes) {
            if (rle_cursor_next(&cur, &b) != 0) {
                break;
            }
            unsigned char run_sym;
            unsigned char run_len;
            rle_decode_byte(b, &run_sym, &run_len);

            long run_end = pos + run_len;

            if (run_end <= i) {
                if (run_sym == symbol) {
                    count += run_len;
                }
                pos = run_end;
                run++;
            } else {
                if (run_sym == symbol) {
                    count += (i - pos);
                }
                break;
            }
        }
    }

    return count;
}

long index_select(const BwtIndex *index, unsigned char symbol, long k) {
    if (k <= 0) {
        return -1;
    }
    long sample_idx = findSampleIndexBySymbolCount(index, symbol, k);

    long pos = index->samples[sample_idx].text_pos_before;
    long run = index->samples[sample_idx].run_index;
    long count = (long) index->samples[sample_idx].count_before[symbol];
    unsigned char b;
    RleCursor cur;
    rle_cursor_init(&cur, index->file, run);

    while (run < index->file->num_bytes) {
        if (rle_cursor_next(&cur, &b) != 0) {
            break;
        }
        unsigned char run_sym;
        unsigned char run_len;
        rle_decode_byte(b, &run_sym, &run_len);

        if (run_sym == symbol) {
            if (count + run_len >= k) {
                long offset = k - count; 
                return pos + offset; 
            }
            count += run_len;
        }

        pos += run_len;
        run++;
    }

    return -1;
}

unsigned char index_char_at(const BwtIndex *index, long i) {
    if (i <= 0 || i > index->file->total_len) {
        return SYM_NL;
    }

    long sample_idx = findSampleIndex(index, i);

    long pos = index->samples[sample_idx].text_pos_before;
    long run = index->samples[sample_idx].run_index;
    unsigned char b;
    RleCursor cur;
    rle_cursor_init(&cur, index->file, run);

    while (run < index->file->num_bytes) {
        if (rle_cursor_next(&cur, &b) != 0) {
            break;
        }
        unsigned char run_sym;
        unsigned char run_len;
        rle_decode_byte(b, &run_sym, &run_len);

        if (pos + run_len >= i) {
            return run_sym;
        }
        pos += run_len;
        run++;
    }

    return SYM_NL;
}

unsigned char index_first_col_symbol(const BwtIndex *index, long i) {

    int order_rev[NUM_SYMS] = { SYM_T, SYM_G, SYM_C, SYM_A, SYM_NL };
    for (int j = 0; j < NUM_SYMS; j++) {
        int s = order_rev[j];
        if (index->C[s] < i) {
            return (unsigned char) s;
        }
    }
    return SYM_NL;
}

static long findSampleIndexBySymbolCount(const BwtIndex *index, unsigned char symbol, long k) {
    long lo = 0;
    long hi = index->num_samples - 1;
    long result = 0;

    while (lo <= hi) {
        long mid = (lo + hi) / 2;
        if ((long) index->samples[mid].count_before[symbol] < k) {
            result = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return result;
}

static long findSampleIndex(const BwtIndex *index, long i) {
    long lo = 0;
    long hi = index->num_samples - 1;
    long result = 0;

    while (lo <= hi) {
        long mid = (lo + hi) / 2;
        if (index->samples[mid].text_pos_before < i) {
            result = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return result;
}

