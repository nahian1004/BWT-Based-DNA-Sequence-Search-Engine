#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rle.h"
#include "index.h"
#include "search.h"

#define INDEX_MEMORY (9L * 1024 * 1024)

static void printUsage(const char *prog) {
    fprintf(stderr, "Usage: %s <rbwt_file> <search_term>\n", prog);
}

static long chooseSampleInterval(long num_runs) {
    long max_samples = INDEX_MEMORY / (long) sizeof(Sample);
    long interval;

    if (max_samples < 1) max_samples = 1;
    interval = num_runs / max_samples + 1;
    return interval < 1 ? 1 : interval;
}

int bwtsearch(const char *rbwt_file, const char *pattern) {
    RbwtFile file;
    BwtIndex index;
    SearchRange range;
    char *context;
    int m;
    long interval;

    if (rbwt_file == NULL || pattern == NULL) return EXIT_FAILURE;

    m = (int) strlen(pattern);
    if (m <= 0) return 0;

    if (rle_open(rbwt_file, &file) != 0) return EXIT_FAILURE;

    for (int i = 0; i < m; i++) {
        if (rle_char_to_symbol(pattern[i]) < 0) {
            rle_close(&file);
            return 0;
        }
    }

    interval = chooseSampleInterval(file.num_bytes);
    if (index_build(&file, interval, &index) != 0) {
        rle_close(&file);
        return EXIT_FAILURE;
    }

    range = search_backward(&index, pattern, m);

    context = malloc((size_t) m + 5);
    if (!context) {
        index_free(&index);
        rle_close(&file);
        return EXIT_FAILURE;
    }

    for (long row = range.first; row <= range.last; row++) {
        int len = search_extract_context(&index, row, pattern, m, context, (size_t) m + 5);
        if (len > 0) {
            fwrite(context, 1, (size_t) len, stdout);
            fputc('\n', stdout);
        }
    }

    free(context);
    index_free(&index);
    rle_close(&file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) { printUsage(argv[0]); return 1; }
    return bwtsearch(argv[1], argv[2]);
}
