#! /usr/bin/python3

import os
import sys
import unittest
import cffi

import test_overlap
from test_overlap import *

ec = os.system('gcc -Wall -shared -o liboverlap_algo.so overlap_algo.c')
if ec:
    sys.exit(ec)

os.environ['CPLUS_INCLUDE_PATH'] = '/usr/include/python3.9'

ffi = cffi.FFI()
ffi.cdef(os.popen('gcc -E overlap_algo.h').read())
ffi.dlopen('liboverlap_algo.so')

if 0:
    ffi.set_source(
            'overlap_algo',
            '#include "overlap_algo.h"',
            libraries = [ 'overlap' ],
            library_dirs = [ os.getcwd() ],
            extra_link_args = [ '-Wl,-rpath,.' ],
    )
    ffi.compile(verbose = 'True')

class COverlapAlgorithm(object):
    def __init__(self):
        self.algo = ffi.overlap_new()

test_overlap.ALGOS.append(COverlapAlgorithm)

def main():
    print(dir (ffi))
    algo = COverlapAlgorithm()

# unittest.main(verbosity = 2)


if __name__ == '__main__':
    print()
    main()

