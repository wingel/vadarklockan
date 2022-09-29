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
            edges.append((resp.adjustment - resp.uncertainty, -1))
            edges.append((resp.adjustment + resp.uncertainty, +1))

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

        self.st = repl['start_time']
        self.rt = self.st + repl['rtt']
        self.remote = repl['datetime']
        # TODO check that radi >= 0
        self.radi = repl['radi']*1E-6
        self.rtt  = repl['rtt']

        self.local = datetime.utcfromtimestamp((self.st + self.rt) / 2)
        self.uncertainty = self.radi + self.rtt / 2

        # How much to adjust the local clock to set it to match the
        # remote clock
        self.adjustment = (self.remote - self.local).total_seconds()

        statf.write(json.dumps({
            'server': repr(self),
            'st' : self.st,
            'rt' : self.rt,
            'remote' : self.remote.replace(tzinfo=timezone.utc).timestamp(),
            'radi' : self.radi,
            'rtt' : self.rtt,
            }) + '\n')
        statf.flush()

def main():
    # Query a list of servers in a JSON file.
    with open('ecosystem.json') as f:
        servers = json.load(f)['servers']

    cl = RoughtimeClient()

    servers = [ Server(cl, _) for _ in servers ]

    # servers = servers[:6]

    # Try all servers a couple of times simulating that we have more
    servers = servers * 3

    random.shuffle(servers)

    algorithm = SelectionAlgorithm(10)

    statf.write('\n')
    statf.flush()

    last_wanted = 0
    for server in servers:
        if 0 and server.addr.startswith('false'):
            continue

        try:
            server.query()

            print('%-38s local %s remote %s adj %9.0f us rtt %7.0f us radi %7.0f us' % (
                server, server.local, server.remote, server.adjustment * 1E6, server.rtt * 1E6, server.radi * 1E6))

        except Exception as e:
            print('%-38s Exception: %s' % (server, e))
            continue

        if algorithm.handle(server):
            print()
            print("success with %u/%u clocks adjust %+.0f us uncertainty %.0f us" % (
                algorithm.wanted, len(algorithm.responses),
                algorithm.adjustment * 1E6,
                algorithm.uncertainty * 1E6))

            if 1:
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

            assert algorithm.adjustment < 1E-3

if __name__ == '__main__':
    main()
