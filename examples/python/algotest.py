#! /usr/bin/python3
# -*- coding: utf-8 -*-

from __future__ import division, print_function, unicode_literals

"""An example on how to use pyroughtime with the overlap algoritm from
"vad är klockan" to set the clock."""

import os
import sys
import json
import random
from datetime import datetime, timedelta, timezone

if __name__ == '__main__' and not sys.argv[0]:
    print()

from pyroughtime import RoughtimeClient

from overlap import OverlapAlgorithm
from overlap_ref import ReferenceOverlapAlgorithm

statf = open('stat.json', 'a')

class Server(object):
    """Glue betweeen roughtime and measurements handed to the algorithm"""

    def __init__(self, cl, j):
        self.cl = cl
        self.proto = j['addresses'][0]['protocol']
        assert j['publicKeyType'] == 'ed25519'
        assert self.proto in [ 'udp', 'tcp' ]
        self.newver = j.get('newver', False)
        self.addr, self.port = j['addresses'][0]['address'].split(':')
        self.port = int(self.port)
        self.pubkey = j['publicKey']

    def __repr__(self):
        return "%s:%s" % (self.addr, self.port)

    def query(self):
        repl = self.cl.query(self.addr, self.port, self.pubkey, 2, self.newver, self.proto)

        self.transmit_time = repl['start_time']
        self.receive_time = self.transmit_time + repl['rtt']
        self.remote_time = repl['datetime']
        # TODO check that radi >= 0
        self.radi = repl['radi']*1E-6

        statf.write(json.dumps({
            'server': repr(self),
            'transmit_time' : self.transmit_time,
            'receive_time' : self.receive_time,
            'remote_time' : self.remote_time.replace(tzinfo=timezone.utc).timestamp(),
            'radi' : self.radi,
            }) + '\n')
        statf.flush()

    def get_meas(self):
        # Round trip time
        rtt = self.receive_time - self.transmit_time

        # Local clock midway between transmit_time and receive_time
        local_time = datetime.utcfromtimestamp(self.transmit_time + rtt / 2)

        # Calculate a local clock adjustment
        adjustment = (self.remote_time - local_time).total_seconds()

        # The uncertainty is half the round trip time plus the server uncertainty (radi)
        uncertainty = rtt / 2 + self.radi

        return local_time, rtt, adjustment, uncertainty

    def get_adjustment_range(self):
        """Return an adjustment range to overlap algorithm"""
        local_time, rtt, adjustment, uncertainty = self.get_meas()

        # Return lowest and highest adjustments
        return adjustment - uncertainty, adjustment + uncertainty

def main():
    # Query a list of servers in a JSON file.
    with open('ecosystem.json') as f:
        servers = json.load(f)['servers']

    cl = RoughtimeClient()

    servers = [ Server(cl, _) for _ in servers ]

    # servers = servers[:6]

    # Try all servers a couple of times simulating that we have more
    # The first 7 servers should be good, the rest are falsetickers,
    # so use the good ones more often so that we will have a majority
    # of good ones.
    servers = servers[:7] * 3 + servers * 2

    random.shuffle(servers)

    # We want a majority where at least this many servers agree
    wanted = 10

    algorithm = OverlapAlgorithm()
    ref_algorithm = ReferenceOverlapAlgorithm()

    statf.write('\n')
    statf.flush()

    responses = 0
    last_count = 0

    for server in servers:
        if 0 and server.addr.startswith('false'):
            continue

        if 0 and 'netnod' not in server.addr:
            continue

        try:
            server.query()

            local_time, rtt, adjustment, uncertainty = server.get_meas()

            print('%-38s local %s remote %s adj %9.0f us rtt %7.0f us radi %7.0f us' % (
                server, local_time, server.remote_time,
                adjustment * 1E6, rtt * 1E6, server.radi * 1E6))

        except Exception as e:
            print('%-38s Exception: %s' % (server, e))
            continue

        responses += 1
        lo, hi = server.get_adjustment_range()
        algorithm.add(lo, hi)
        ref_algorithm.add(lo, hi)
        count, lo, hi = algorithm.find()
        ref_count, ref_lo, ref_hi = ref_algorithm.find()
        assert count, ref_count
        assert lo == ref_lo
        assert hi == ref_hi
        if count > responses // 2 and count >= wanted:
            adjustment = (hi + lo) / 2
            uncertainty = (hi - lo) / 2

            print("success with %u/%u clocks adjust %+.0f us uncertainty %.0f us" % (
                count, responses,
                adjustment * 1E6,
                uncertainty * 1E6))
            print()

            # This is a good place to set the clock and break out of the loop

            # Getting more responses should always give more overlaps, never fewer
            assert last_count <= count
            last_count = count

            # assert algorithm.adjustment < 1E-3

if __name__ == '__main__':
    main()
