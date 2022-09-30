#! /usr/bin/python3

import os
import sys
import unittest
import cffi

import test_overlap
from test_overlap import *

# Build a library with the C code we want to test
ec = os.system('gcc -Wall -g -shared -o liboverlap_algo.so overlap_algo.c')
if ec:
    sys.exit(ec)

# Create a CFFI interface to the library
ffi = cffi.FFI()
ffi.cdef(os.popen('gcc -E overlap_algo.h').read())
lib = ffi.dlopen('./liboverlap_algo.so')

# Wrap the library with the same API as the Python implementations
class COverlapAlgorithm(object):
    def __init__(self):
        self.algo = lib.overlap_new()

    def add(self, lo, hi):
        if not lib.overlap_add(self.algo, lo, hi):
            raise ValueError("invalid parameters to add")

    def find(self):
        lo_p = ffi.new('double [1]')
        hi_p = ffi.new('double [1]')
        r = lib.overlap_find(self.algo, lo_p, hi_p)
        if r:
            return r, lo_p[0], hi_p[0]
        else:
            return 0, None, None

    def __del__(self):
        lib.overlap_del(self.algo)

test_overlap.ALGOS.append(COverlapAlgorithm)

def main():
    unittest.main(verbosity = 2)

if __name__ == '__main__':
    print()
    main()

