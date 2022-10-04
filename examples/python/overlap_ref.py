#! /usr/bin/python3

class ReferenceOverlapAlgorithm(object):
    """Optimized algoritm to find an overlapping range.

    Ranges are first added using :py:func:`add()`.  To find the
    overlapping range, call :py:func:`find`:

    >>> from overlap import OverlapAlgorithm
    >>> algo = OverlapAlgorithm()
    >>> algo.add(1, 5)
    >>> algo.add(3, 9)
    >>> count, lo, hi = algo.find()
    >>> print((count, lo, hi))
    (2, 3, 5)

    The first value returned by the method, count, is the number of
    overlaps.  The second and third, lo and hi, are the common overlap
    of all ranges.

    It's possible to iteratively add additional ranges and call
    :py:func:`find` again:

    >>> algo.add(4, 5)
    >>> print(algo.find())
    (3, 4, 5)

    If there are multiple overlapping ranges the one with the highest
    number of overlaps will be returned:

    >>> algo.add(10, 20)
    >>> algo.add(12, 18)
    >>> print(algo.find())
    (3, 4, 5)

    If there are multiple overlapping ranges with the same number of
    overlaps, the boundaries of both ranges will be returned,

    >>> algo.add(13, 17)
    >>> print(algo.find())
    (3, 4, 17)

    It's up to the user to interpret this properly.  One userfule
    requirement is that a majority of the ranges have to overlap.

    >>> ranges = [ (1,5), (3,9), (4,5), (10,20), (12,18) ]
    >>> algo = OverlapAlgorithm(ranges)
    >>> count, lo, hi = algo.find()
    >>> print(len(ranges))
    5
    >>> print((count, lo, hi))
    (3, 4, 5)
    >>> assert count > len(ranges) // 2

    This algorithm is based on the selection & clustering algorithm
    from RFC5905 Appendix A.5.5.1.  Note that this implementation is
    optimized and assumes that adding one more measurement will never
    increase the number of overlaps by more than one.  For a slightly
    simpler algorithm which closer resembles the one in RFC5905 see
    :py:obj:`overlap_ref.ReferenceOverlapAlgorithm`.
    """

    def __init__(self):
        """Create a new ReferenceOverlapAlgorithm object.

        :param ranges: Optional list of "ranges" that should be added.
        """

        self._edges = []
        self._responses = 0

    def add(self, lo, hi):
        """Add a range.

        :param lo: low limit for the range.
        :param hi: high limit for the range.
        :raise ValueError: if lo >= hi
        """

        if lo > hi:
            raise ValueError("lo must not be higher than hi")

        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        self._responses += 1

    def find(self):
        """Find overlapping range.

        :return: count, lo, hi

        count is the number of overlaps.  lo and hi are the limits of
        the overlapping range.
        """

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
