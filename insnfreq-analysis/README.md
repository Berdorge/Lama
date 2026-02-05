## Correctness

The analyzer has been tested through
```bash
python3 test.py
```
which includes test cases from
- `performance`
- `regression` (those that compile)
- `regression_long/expressions`
- `regression_long/deep-expressions`

The expected outputs are not computed. The test simply checks that
the program does not crash.

The output for `performance/Sort.bc` is presented in
`Sort-output.txt`.

## Occurrence counting

All instructions are viewed as byte sequences.

A sequence of two instructions is viewed 
simply as a result of concatenation of two sequences corresponding to
each instruction. This results in a byte sequence as well.
Note that it cannot be interpreted as a single instruction
byte sequence. This allows us to store both single instructions
and instruction pairs in the same dictionary.

An open addressing hashtable is used as a dictionary,
because it is easy to reason about its memory footprint.
It can be allocated at the very beginning of the execution.

The probing is linear. Other probings I've tried
haven't yielded significant performance benefits.
Even though profiling shows that
`hashtable::mark_occurrence`
(without including `equals` time mentioned below)
takes about 33% of the total execution time.

Every hashtable entry stores an instruction pointer,
the instruction length and the occurrence count.
It means that sometimes, in order to compare the keys,
we need to read bytes from the code.
But I believe this is a reasonable trade-off
for memory efficiency. The profiling
shows that `equals` takes about 9% of the total execution time.

## Analyzer abstraction

The `analyzer` is abstracted away from
instruction encoding through a `Handler` template parameter.
If I am not mistaken,
this is a no-cost abstraction
(compared to a simple `switch` statement).

The things I decided to not abstract away are:
- Reading public area. It is Lama-dependent and takes a bit of code.
- Determining maximum hash table size. It is Lama-dependent
  (more about it in [Memory Usage](#memory-usage) section)
  and takes a bit of code.

## Memory Usage

Let's calculate the maximum amount of entries 
in the hashtable, given a program of size N bytes.
There are at most `256` unique single-byte instructions,
hence at most `65536` unique two-byte instruction pairs.

Other instructions take at least 5 bytes, so there are
at most `N / 5 + 256` unique entries for single instructions.

There are at most `N - 1` pairs of adjacent instructions,
and the smallest pair larger than two bytes takes `1 + 5 = 6` bytes.
Since pairs overlap, we can have at most `N / 3 + 65536` unique pairs of instructions.

Thus, the total number of unique entries
is at most
`N / 5 + 256 + N / 3 + 65536`. Multiplying by 12
bytes (size of each entry), we get
`6.4N + 789504` bytes. Let's also add a bit of headroom
so that the load factor is at most `3/4`.
Finally, we get that the hashtable size
is at most `8.53N + 1052672` bytes, which for
`N` larger than `2239727` (about 2 MB) is at most `9N` bytes.

The code itself takes `N` bytes.

`worklist` takes at most `N` bytes for the content
(worst case: every instruction is a jump)
and at most `N` bytes for the `std::vector` capacity overhead.

`visited` and `is_flow_continued` bitset each take at most `N / 8` bytes.

Finally, the total memory usage, for files that are not too small,
is at most `12N`.

## Performance

I got the following results on my machine:
```bash
$ python3 generate.py 1000000000 > 1gb.bc
$ time build/lama-insnfreq-analysis --input 1gb.bc --threshold 100000000
build/lama-insnfreq-analysis --input 1gb.bc --threshold 1000000000  40,61s user 1,99s system 99% cpu 42,621 total
```
The max memory usage was 10 GB.
