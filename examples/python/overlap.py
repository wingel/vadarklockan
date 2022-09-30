#! /usr/bin/python3

class OverlapAlgorithm(object):
    """Algoritm from RFC5905 A.5.5.1 to find an overlapping range.

    This is an iterative process, add a new range and return the
    number of overlaps.  If the number of overlaps is >0 the members
    .lo and .hi will contain be the overlap.

    This is an optimized version which relies on the fact that adding
    one more measurement will never increase the number of overlaps by
    more than one.

    """

    def __init__(self):
        self._edges = []
        self._wanted = 0

    def add(self, lo, hi):
        if lo > hi:
            raise ValueError("lo must not be higher than hi")

        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        # Increase the number of possible overlaps
        self._wanted += 1

    def find(self):
        if not self._wanted:
            return 0, None, None

        while self._wanted:
            chime = 0
            lo = None
            for e in self._edges:
                chime -= e[1]
                if chime >= self._wanted:
                    lo = e[0]
                    break

            chime = 0
            hi = None
            for e in reversed(self._edges):
                chime += e[1]
                if chime >= self._wanted:
                    hi = e[0]
                    break

            if lo is not None and hi is not None and lo <= hi:
                break

            self._wanted -= 1

        return self._wanted, lo, hi
