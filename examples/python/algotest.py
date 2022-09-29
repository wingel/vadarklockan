#! /usr/bin/python3
# -*- coding: utf-8 -*-

from __future__ import division, print_function, unicode_literals

""" Försök till clock selection algorithm implementerad i Python som
ska illustrera de viktiga koncepten för en implementation i andra
språk.

Mål:

ska helst gå att köra singeltrådat
implementationen kanske kan vara trådad under skalet.

"""

import os
import sys
import json
import random
from datetime import datetime, timedelta, timezone

if __name__ == '__main__' and not sys.argv[0]:
    print()

from pyroughtime import RoughtimeClient

statf = open('stat.json', 'a')

class SelectionAlgorithm(object):
    def __init__(self, n):
        self.n = n
        self.responses = []
        self.bad = 0

    def handle(self, resp):
        """Algoritm from RFC5905 A.5.5.1 to find the minimum overlap of
        adjustments.

        Note, this implementation could be optimized.  The list of
        edges does not have to be recalculated each time.  A simple
        insertion sort could add more entries to the list.

        I believe that the last "allow" value could also be reused.
        Adding one more measurement should best case make the last
        allow value work, it will never make allow decrement.
        TODO Have to verify this assumption.

        """

        self.responses.append(resp)
        edges = []
        for resp in self.responses:
            lo, hi = resp.get_adjustment_range()

            edges.append((lo, -1))
            edges.append((hi, +1))

        edges.sort()

        max_allow = len(self.responses) - self.n
        if max_allow < 0:
            return False

        for allow in range(max_allow + 1):
            self.wanted = len(self.responses) - allow

            chime = 0
            lo = None
            for e in edges:
                chime -= e[1]
                if chime >= self.wanted:
                    lo = e[0]
                    break

            chime = 0
            hi = None
            for e in reversed(edges):
                chime += e[1]
                if chime >= self.wanted:
                    hi = e[0]
                    break

            if lo is not None and hi is not None and lo <= hi:
                break

        else:
            return False

        self.adjustment = (lo + hi) / 2
        self.uncertainty = (hi - lo) / 2

        return True

class Server(object):
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

    def get_adjustment_range(self):
        rtt = self.receive_time - self.transmit_time
        local_time = datetime.utcfromtimestamp(self.transmit_time + rtt / 2)
        adjustment = (self.remote_time - local_time).total_seconds()
        uncertainty = rtt / 2 + self.radi

        return adjustment - uncertainty, adjustment + uncertainty

def main():
    # Query a list of servers in a JSON file.
    with open('ecosystem.json') as f:
        servers = json.load(f)['servers']

    cl = RoughtimeClient()

    servers = [ Server(cl, _) for _ in servers ]

    # servers = servers[:6]

    # Try all servers a couple of times simulating that we have more
    servers = servers * 5

    random.shuffle(servers)

    algorithm = SelectionAlgorithm(10)

    statf.write('\n')
    statf.flush()

    last_wanted = 0
    for server in servers:
        if 1 and server.addr.startswith('false'):
            continue

        if 0 and 'netnod' not in server.addr:
            continue

        try:
            server.query()

            rtt = server.receive_time - server.transmit_time
            local_time = datetime.utcfromtimestamp(server.transmit_time + rtt / 2)

            # How much to adjust the local clock to set it to match the
            # remote clock, assuming that the delays are symmetric
            adjustment = (server.remote_time - local_time).total_seconds()

            uncertainty = rtt / 2 + server.radi

            print('%-38s local %s remote %s adj %9.0f us rtt %7.0f us radi %7.0f us' % (
                server, local_time, server.remote_time,
                adjustment * 1E6, rtt * 1E6, server.radi * 1E6))

        except Exception as e:
            print('%-38s Exception: %s' % (server, e))
            continue

        if algorithm.handle(server):
            print()
            print("success with %u/%u clocks adjust %+.0f us uncertainty %.0f us" % (
                algorithm.wanted, len(algorithm.responses),
                algorithm.adjustment * 1E6,
                algorithm.uncertainty * 1E6))

            if 0:
                for resp in algorithm.responses:
                    if resp.adjustment + resp.uncertainty < algorithm.adjustment - algorithm.uncertainty:
                        if not repr(resp).startswith('falseticker'):
                            print("lo: %s" % resp)
                            raise ValueError()
                    elif resp.adjustment - resp.uncertainty > algorithm.adjustment + algorithm.uncertainty:
                        if not repr(resp).startswith('falseticker'):
                            print("hi: %s" % resp)
                            raise ValueError()
                    else:
                        # print("ok: %s" % resp)
                        pass

            # assert last_wanted <= algorithm.wanted
            last_wanted = algorithm.wanted

            # assert algorithm.adjustment < 1E-3

if __name__ == '__main__':
    main()
