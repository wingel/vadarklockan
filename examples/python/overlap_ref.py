#! /usr/bin/python3

class RefOverlapAlgorithm(object):
    """Algoritm from RFC5905 A.5.5.1 to find an overlapping range.

    This is an iterative process, add a new range and return the
    number of overlaps.  If the number of overlaps is >0 the members
    .lo and .hi will contain be the overlap.
    """

    def __init__(self):
        self._edges = []
        self._responses = 0

    def add(self, lo, hi):
        if lo > hi:
            raise ValueError("lo must not be higher than hi")

        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        self._responses += 1

    def find(self):
        if not self._responses:
            return 0, None, None

        for allow in range(self._responses):
            wanted = self._responses - allow

            chime = 0
            lo = None
            for e in self._edges:
                chime -= e[1]
                if chime >= wanted:
                    lo = e[0]
                    break

            chime = 0
            hi = None
            for e in reversed(self._edges):
                chime += e[1]
                if chime >= wanted:
                    hi = e[0]
                    break

            if lo is not None and hi is not None:
                break

        return wanted, lo, hi
