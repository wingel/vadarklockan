# Vad är klockan implementation in C

overlap_algorithm.[ch] is a port of the Python implementation in
../python/overlap.py to C.

# Test cases

test_overlap_c.py reuses the test cases from the Python
implementation.  The C code is built into a dynamic library which is
wrapped with a class which implements the same API as the Python
variant.  The results of the Python and C implementations are
compared.  Additionally, some C code to replay the same test caes is
generated to overlap_replay.c which is then compiled and run through
valgrind to catch buffer overruns or memory leaks.

To run the test cases, install the following packages on a Debian or
Ubuntu system:

    apt-get install -y build-essential python3-cffi valgrind

Then run the test cases with:

    python3 test_overlap_c.py
