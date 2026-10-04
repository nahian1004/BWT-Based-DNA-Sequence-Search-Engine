#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"

static int appendChar(char *buf, int *out_end, size_t buf_size, char c);

SearchRange search_backward(const BwtIndex *index, const char *pattern, int m) {
    SearchRange range;
    unsigned char c;
    int i;

    if (index == NULL || pattern == NULL || m <= 0) {
        range.first = 1;
        range.last = 0;
        return range;
    }

    c = (unsigned char) rle_char_to_symbol(pattern[m - 1]);
    range.first = index->C[c] + 1;
    range.last = index->C[c] + index->counts[c];

    for (i = m - 2; i >= 0 && range.first <= range.last; i--) {
        c = (unsigned char) rle_char_to_symbol(pattern[i]);
        long occ_lo = index_occ(index, c, range.first - 1);
        long occ_hi = index_occ(index, c, range.last);
        range.first = index->C[c] + occ_lo + 1;
        range.last  = index->C[c] + occ_hi;
    }

    return range;
}

long search_LF(const BwtIndex *index, long i) {
    unsigned char c = index_char_at(index, i);
    return index->C[c] + index_occ(index, c, i);
}

long search_FL(const BwtIndex *index, long i) {
    unsigned char c = index_first_col_symbol(index, i);
    long k = i - index->C[c];
    return index_select(index, c, k);
}

int search_extract_context(const BwtIndex *index, long row, const char *pattern,
                            int m, char *buf, size_t buf_size) {
    char before[2];
    int n_before = 0;
    char after[2];
    int n_after = 0;
    long r;
    int j;
    int total = 0;

    r = row;
    for (j = 0; j < 2; j++) {
        unsigned char sym = index_char_at(index, r);
        if (sym == SYM_NL) break;
        before[n_before++] = rle_symbol_to_char(sym);
        r = search_LF(index, r);
    }

    r = row;
    for (j = 0; j < m; j++) {
        r = search_FL(index, r);
    }
    for (j = 0; j < 2; j++) {
        unsigned char sym = index_first_col_symbol(index, r);
        if (sym == SYM_NL) break;
        after[n_after++] = rle_symbol_to_char(sym);
        r = search_FL(index, r);
    }

    for (j = n_before - 1; j >= 0 && (size_t) total < buf_size; j--) {
        appendChar(buf, &total, buf_size, before[j]);
    }
    for (j = 0; j < m && (size_t) total < buf_size; j++) {
        appendChar(buf, &total, buf_size, pattern[j]);
    }
    for (j = 0; j < n_after && (size_t) total < buf_size; j++) {
        appendChar(buf, &total, buf_size, after[j]);
    }

    return total;
}

static int appendChar(char *buf, int *out_end, size_t buf_size, char c) {
    if ((size_t) *out_end >= buf_size) {
        return -1;
    }
    buf[*out_end] = c;
    *out_end += 1;
    return 0;
}
