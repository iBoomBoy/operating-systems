# Assignment 2: Parallel Merge Sort

* Authors: Yiran Bi, Ajay Ramesh
* Group name: Group-74

## Overview

This program sorts an array of integers using merge sort in C.
It uses POSIX threads to sort separate parts of the array concurrently,
with a configurable cutoff level that controls when sorting switches
to the single-threaded implementation.

## Manifest

- mergesort.c: Implements merging, single-threaded sorting, parallel
  sorting, and thread argument construction.
- mergesort.h: Declares the function interfaces, shared arrays, and
  thread argument structure.
- test-mergesort.c: Generates random input, runs the sorting function,
  checks whether the result is ordered, and reports elapsed time.
- Makefile: Defines the commands used to compile and clean the project.
- README.template: Original documentation template supplied with the project.
- README: Describes the implementation, usage, and testing.

## Building the project

A C compiler, make, and POSIX thread support are required.
Open a terminal in the directory containing the Makefile and run:

    make clean && make

This creates the test-mergesort executable. We run `make clean` first
because the given Makefile links with `$?` (only the files that changed),
so running `make` after editing just one file can give a link error.

To run a single-threaded test:

    ./test-mergesort 100 0 1234

On macOS, comment out `#include <error.h>` in test-mergesort.c before
compiling. This header is not used by the test program. The submitted
version keeps the line. On Windows, use WSL (Ubuntu).

To remove generated build files:

    make clean

## Features and usage

Run the program with:

    ./test-mergesort <input_size> <cutoff_level> <seed>

- input_size: Number of randomly generated integers to sort; the supplied
  test program requires at least two.
- cutoff_level: Thread depth at which sorting switches to single-threaded
  merge sort. A value of 0 runs the single-threaded version. Cutoff c gives
  2^c threads doing the sorting at the bottom level, and 2^(c+1) - 2
  threads created in total.
- seed: Seed used to generate the random input.

For example:

    ./test-mergesort 100 0 1234
    ./test-mergesort 100 2 1234

The single-threaded implementation recursively sorts the left and right
halves of an array interval and merges them using temporary array B.

The parallel implementation assigns the halves to child threads until
the cutoff level is reached. A parent waits for its children to finish
before merging their results. Thread arguments (left, right, level) are
packed into a struct by buildArgs(), because a thread function can only
take one pointer.

Concurrency and safety:

* No locks are needed. Each thread only works on its own range of `A` and
  the same range of `B`, and sibling ranges never overlap. A parent only
  touches its range after `pthread_join` confirms both children finished.
* Both threads are created before either is joined, so they really run at
  the same time.
* Memory: a parent frees its children's arguments after joining them. The
  level-0 argument is freed by the test program, so `parallel_mergesort`
  never frees its own argument (that would be a double free).
* If `pthread_create` fails (for example the OS thread limit is reached),
  the current thread sorts that half itself, so the array is still sorted.
* If `malloc` fails in `buildArgs`, the program prints an error and exits
  instead of using a NULL pointer.
* The middle index is worked out as `left + (right - left) / 2` to avoid
  integer overflow.

## Testing

**Correctness.** We ran the program with many sizes - 2, 3, 5, 7, 10, 100,
1001, 65537 and 100,000,000, cutoffs 0 to 8 and extreme cutoff like 19, 20, 
and different seeds.
The test program checks every result is sorted, and every run passed.
We also tried odd sizes and very small arrays (2 and 3 elements) with high
cutoffs, so some threads get 0 or 1 elements.

**Memory and thread checks.** These tools were new to us, so we asked
Claude for help with this part. Claude suggested and ran checks with
AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer and Valgrind,
and none of them reported memory leaks, double frees, out-of-bounds access
or data races. Then we ran ThreadSanitizer in WSL
(`./tsan 300000 4 7`, 30 threads) and it reported no data races. On WSL it
had to be run with `setarch $(uname -m) -R` because ThreadSanitizer clashes
with the kernel's address randomisation (ASLR).

**Thread failure.** Also with Claude's help, the fallback was tested by
making `pthread_create` fail on purpose every third call. The array was
still sorted correctly.

**Performance.** 100,000,000 elements, seed 1234, on three computers.
Speedup = time at cutoff 0 / time at cutoff c.

| Cutoff | i7-14650HX (WSL) | Ajay's Mac | Yiran's Mac |
|--------|------------------|------------|----------------|
| 0      | 12.87 s (1.00x)  | 14.66 s (1.00x) | 15.29 s (1.00x) |
| 1      | 6.67 s (1.93x)   | 7.57 s (1.94x)  | 7.86 s (1.95x)  |
| 2      | 3.49 s (3.69x)   | 4.06 s (3.61x)  | 4.35 s (3.51x)  |
| 3      | 2.06 s (6.25x)   | 2.50 s (5.86x)  | 2.86 s (5.35x)  |
| 4      | 1.46 s (8.82x)   | 2.26 s (6.49x)  | 2.77 s (5.52x)  |
| 5      | 1.25 s (10.30x)  | 2.26 s (6.49x)  | 2.96 s (5.17x)  |
| 6      | 1.22 s (10.55x)  | 2.27 s (6.46x)  | 2.86 s (5.35x)  |
| 7      | 1.21 s (10.64x)  | 2.24 s (6.54x)  | 3.50 s (4.37x)  |
| 8      | 1.22 s (10.55x)  | 2.41 s (6.08x)  | 2.83 s (5.40x)  |
| 9      | -                | 2.23 s (6.57x)  | -               |

* i7-14650HX laptop: 16 cores (8 performance + 8 efficiency), 24 logical
  processors, Ubuntu on WSL.
* Ajay's Mac and Yiran's Mac: Apple Silicon MacBooks, macOS.

Earlier, cutoff 15 on Ajay's Mac took 7.81 s and 8.68 s (about 1.7-1.9x).

Stress tests with very high cutoffs (i7, WSL). We ran these ourselves and
asked Claude to help explain the results:

| Test                       | Result                        |
|----------------------------|-------------------------------|
| 3 elements, cutoff 15      | sorted correctly, 0.00 s      |
| 100,000,000, cutoff 19     | sorted correctly, 21.56 s (0.60x) |
| 100,000,000, cutoff 20     | sorted correctly, 34.24 s (0.38x) |

Cutoff 19 asks for about 1 million threads and cutoff 20 about 2 million,
so the program becomes slower than the single-threaded version. It still
sorts correctly, because if a thread cannot be created the current thread
does that half itself. The 3-element test shows that ranges with 0 or 1
elements are handled.

What we saw:

* All three computers pass the required 2x speedup from cutoff 2 upwards,
  and the best speedup was 10.64x on the i7 chip.
* Cutoff 1 gives just under 2x everywhere. Two threads can at
  best halve the time, and the last merge of the whole array is still done
  by one thread, plus there is some thread overhead.
* Each computer stops improving at a different cutoff: around 3-4 on the
  Macs and around 5 on the i7. This matches the number of cores. The i7 has
  the most cores, so it keeps getting faster for longer and reaches the
  biggest speedup. Once every core is busy, extra threads just wait.
* After that point the times go up and down a little between cutoffs (for
  example 3.50 s at cutoff 7 on Yiran's Mac). This is because the OS
  schedules the threads differently each run, and memory speed is shared by
  all cores, so it becomes the limit.
* At very high cutoffs like 15, which is 65,534 threads it gets much slower.
  Creating and switching between that many threads costs a lot, and macOS
  limits threads per process, so many `pthread_create` calls fail and those
  halves are sorted without a new thread.

## Known Bugs

* This code doesn't work in Windows but apart from that it works in all 
  Linux systems and macOS.
* Very high cutoffs work but are slower, for the reasons above.
* The given Makefile uses `$?` when linking, so always run `make clean`
  before `make`.

## Reflection and Self Assessment

### Ajay

At the start I thought merge sort was mostly about splitting the array, and
I couldn't see where the actual sorting happened. What clicked for me was
that splitting doesn't sort anything. It just breaks the array down into
single elements, which are already sorted, and all the real sorting happens
in `merge()`, where the smaller front element is picked each time. Once I
got that, `parallel_mergesort()` made sense: it has the same shape as
`my_mergesort()`, but the two halves are handed to new threads so they are
sorted at the same time on different cores.

I also had to understand what the cutoff does. Each split only ever creates
two threads, and the cutoff decides how many levels of splitting get threads.
After that, each thread sorts its piece with normal merge sort. This is
needed because creating a thread costs much more than sorting a few numbers,
and there are only so many cores.

One thing I questioned while working on this was the number of threads.
With cutoff c, the total number of threads created is 2 + 4 + ... + 2^c =
2^(c+1) - 2, but only the 2^c threads at the bottom level are actually
sorting. The parent threads above them are not finished after splitting:
they wait in `pthread_join()` and then merge their children's results. While
they wait they are blocked and use no CPU, so at any moment only about 2^c
threads are really working. This made me think the design could be improved:
instead of creating two threads and then sitting idle, a parent could create
one thread for the left half and sort the right half itself. That would
create fewer threads while keeping the same number working at once. We kept
the two-threads-per-split design because it is what the specification and
its flow diagram describe, but I think it is a fair improvement.

Testing on different computers taught me that more threads is not always
faster. Each computer stopped improving at a different cutoff, depending on
how many cores it had, and very high cutoffs (19 and 20, with about 1 to 2
million threads) were slower than not using threads at all.

I also ran into build problems. On Windows the test program would not
compile because `sys/times.h` and pthreads are not available, so I set up
WSL. On macOS I first compiled only `test-mergesort.c` and got "undefined
symbols" errors, because `mergesort.c` was not linked in. Using the Makefile
fixed this. When I ran ThreadSanitizer in WSL it crashed with an "unexpected
memory mapping" error, which turned out to be a clash with ASLR (address
randomisation), and running it with `setarch -R` fixed it.

Testing tools like sanitizers were new to me, so I asked Claude for help with
that part, and I plan to learn them properly before the interview.
Work split: Yiran wrote `merge()`, `my_mergesort()` and
`buildArgs()`. Ajay wrote `parallel_mergesort()`. We both tested the code
and wrote this README.

## Sources Used

* Assignment code from the Github repo provided.
* OSTEP (Operating Systems: Three Easy Pieces), chapters 26 "Concurrency:
  An Introduction" and 27 "Interlude: Thread API".
* Linux man pages: `pthread_create(3)`, `pthread_join(3)`, `malloc(3)`,
  `memcpy(3)`.
* Stackoverflow (https://stackoverflow.com/questions/19072004/understanding-the-recursion-of-mergesort) 
  was used to understand the alogirthm and code better.
* Claude (Model - Opus 5.5) was used to review our code and was 
  asked for further stress testing, this includes including running
  sanitizer/Valgrind checks, the `pthread_create` failure test, and 
  explaining the timing results.
* ChatGPT (Astra - 6) was used for spell correction and grammar correction
  in this README file and for comments spell check as well.
