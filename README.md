# BWT-Based DNA Sequence Search Engine

A pattern-search tool for DNA data, written from scratch in **C**. It searches a **run-length-encoded Burrows–Wheeler Transform (BWT)** directly, without ever decompressing it, using an FM-index-style backward search and a sampled index that fits in a fixed memory budget.

![Language: C](https://img.shields.io/badge/language-C-blue)
![Type: CLI utility](https://img.shields.io/badge/type-CLI%20utility-lightgrey)

## Highlights

- **Searches compressed data in place.** Queries run against the run-length-encoded BWT file itself; the original text is never reconstructed.
- **FM-index backward search.** Finding a pattern of length *m* takes about 2*m* rank queries, independent of how much text there is to scan.
- **Fixed-memory sampled index.** Checkpoints are taken every *k* runs, and *k* is chosen automatically so the index fits in a 9 MB budget however large the input is. This is an explicit space/time trade-off: a larger file means sparser checkpoints and slightly longer scans between them.
- **Context around every hit.** Each match is printed with up to 2 characters either side, recovered by walking the BWT backwards (LF mapping) and forwards (FL mapping via a select query).
- **Modular design.** Separate modules for run-length decoding (`rle`), the checkpointed index (`index`), and the search logic (`search`).

## How it works

The BWT rearranges a text so that identical characters cluster into runs, which run-length encoding then compresses. It can also be searched without being decompressed:

1. **Sorted first column.** Counting how many of each symbol (newline, A, C, G, T) the file contains gives `C[s]`, the number of symbols smaller than `s`.
2. **Backward search.** Starting from the last character of the pattern, the search keeps a range of rows whose suffixes start with the part of the pattern matched so far. Each earlier character narrows the range using two *rank* queries, `occ(c, i)`, which count occurrences of a symbol in the first `i` characters of the BWT.
3. **Rank queries on compressed data.** `occ` jumps to the nearest checkpoint with a binary search, then scans a few runs forward from there.
4. **Printing matches.** For each row left in the range, LF and FL steps recover the neighbouring characters, stopping at line boundaries.

**Checkpoint index.** Each checkpoint stores its run position, the text position, and the running count of every symbol up to that point. The spacing is `runs / max_checkpoints + 1`, where `max_checkpoints` is how many checkpoints fit in 9 MB.

## Input format

The input is a run-length-encoded BWT of a DNA text, where each line ends with a newline. Symbols sort as newline < A < C < G < T. Each run is one byte:

| Bits | Meaning |
|------|---------|
| 7–5 | Symbol: A=0, C=1, G=2, T=3, newline=4 |
| 4–0 | Run length minus 1 (so one byte holds a run of 1 to 32) |

## Build and run

```bash
# Build
make

# Search
./bwtsearch <rbwt_file> <search_term>
```

**Example.** For a BWT of a text with the lines `ACGT`, `TTAC` and `GGA`:

```
$ ./bwtsearch example.rbwt AC
TTAC
ACGT
```

Each output line is one match: up to two characters before it, the pattern, and up to two characters after, never crossing a line boundary.

**Behaviour on unusual input**

- An empty pattern, or one containing characters other than `A`, `C`, `G`, `T` or newline, prints nothing and exits normally.
- A missing or unreadable file, or the wrong number of arguments, prints a usage message or exits with an error code.

## Project structure

```
bwtsearch.c   Entry point: opens the file, builds the index, runs the search, prints matches
rle.c / .h    Run-length file access: byte decoding, buffered cursor over runs
index.c / .h  Checkpointed index: occ (rank), select, character lookup
search.c / .h Backward search, LF/FL mapping, context extraction
Makefile
```

## Limitations

- **Fixed alphabet.** Only A, C, G, T and newline are supported.
- **Index rebuilt on each run.** Every invocation makes one pass over the compressed file to build the checkpoints, so a single query costs a scan of the file. The index is not saved between runs.
- **Rank queries read from disk.** Each rank query re-reads a small block of the file instead of keeping the whole thing in memory. This is what keeps memory use fixed, at the cost of speed.
- **Fixed context size.** Output always shows at most two characters either side of a match.

## Possible extensions

- Save the checkpoint index to disk so repeated queries skip the scan.
- Cache recently read blocks to speed up rank queries.
- Support a larger alphabet, such as IUPAC ambiguity codes (N and others) found in real genomes.
- Add a program that builds the BWT from raw text, so the tool can be run end to end.
- Benchmark query time and memory against a naive scan across input sizes.

## What I learned

- How the BWT and FM-index work, and why backward search needs only rank queries.
- Designing a space/time trade-off: choosing checkpoint spacing from a memory budget.
- Working with compressed data in place, including rank and select over run-length-encoded runs.
- Structuring a C project into small modules with clear interfaces.

## Author

**Nahian Khan**: Commerce (Information Systems) / Computer Science, UNSW
[LinkedIn](https://linkedin.com/in/nahian-khan-b4098525a)
