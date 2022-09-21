import machine, time, network
import binascii
import rt
import json
import sys
import json
import random
import os
import gc

#from __future__ import division, print_function, unicode_literals

cl = rt.RoughtimeClient()

import rt_config

sta = network.WLAN(network.STA_IF)
if not sta.active():
    sta.active(True)

    # print(sta.scan())
    sta.connect(rt_config.WLAN_SSID, rt_config.WLAN_PASSWORD)

while not sta.isconnected():
    pass

print(sta.ifconfig())

# time.sleep(3)


class Edge(object):
    pass

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
            found = 0

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

        host = self.addr
        port = self.port
        pubkey = binascii.a2b_base64(self.pubkey)
        newver = self.newver
        proto = self.proto

        cl = rt.RoughtimeClient()


        repl = cl.query(host, port, pubkey, timeout = 2, newver = newver)

#         repl = self.cl.query(self.addr, self.port, self.pubkey, 2, self.newver, self.proto)



        self.st = repl['st']
        self.rt = repl['rt']
        self.remote = repl['datetime']
        # TODO check that radi >= 0
        self.radi = repl['radi']*1E-6
        self.rtt  = repl['rtt']


        self.local = ((self.st + self.rt) / 2)



        self.uncertainty = self.radi + self.rtt / 2

        # How much to adjust the local clock to set it to match the
        # remote clock
        self.adjustment = self.remote - self.local#.total_seconds()



class testsingle():

    host = 'sth1.roughtime.netnod.se'
    port = 2002
    pubkey = binascii.a2b_base64('9l1JN4HakGnG44yyqyNNCb0HN0XfsysBbnl/kbZoZDc=')
    newver = True

    cl = rt.RoughtimeClient()


    repl = cl.query(host, port, pubkey, timeout = 2, newver = newver)

    print('%s (RTT: %.1f ms)' % (repl['prettytime'], repl['rtt'] * 1000))
    if 0:
        if 'dtai' in repl:
            print('TAI - UTC = %ds' % repl['dtai'])
        if 'leap' in repl:
            if len(repl['leap']) == 0:
                print("Leap events: None")
            else:
                print("Leap events: ")
        print('Delegate key validity start: %s' %
                repr(time.gmtime(repl['mint'])))
        if repl['maxt'] is None:
            print('Delegate key validity end:   indefinite')
        else:
            print('Delegate key validity end:   %s' %
                    repr(time.gmtime((repl['maxt']))))
    #print('Merkle tree path length: %d' % repl['pathlen'])



class testMore():
    # Query a list of servers in a JSON file.
    with open('ecosystem.json') as f:
        servers = json.load(f)['servers']

    cl = rt.RoughtimeClient()

    servers = [ Server(cl, _) for _ in servers ]

#     servers = servers[:10]

#     try all servers a couple of times simulating that we have more
    servers = servers * 3

    def random_shuffle(seq):
        l = len(seq)
        for i in range(l):
            j = random.randrange(l)
            seq[i], seq[j] = seq[j], seq[i]

    random_shuffle(servers)


    algorithm = SelectionAlgorithm(10)

    last_wanted = 0

    for server in servers:
        # Force garbage collection to run
        gc.collect()

        try:
            server.query()


            print('%-38s local %s remote %s adj %9.0f us rtt %7.0f us radi %7.0f us' % (
#                 server, server.local, server.remote, server.adjustment * 1E6, server.rtt * 1E6, server.radi * 1E6))
                server, str(time.gmtime(int(server.local))), str(time.gmtime(int(server.remote))), server.adjustment, server.rtt  * 1E6, server.radi  * 1E6))

        except Exception as e:
            print('%-38s Exception: %s' % (server, e))
            continue

        if algorithm.handle(server):
            print("success with %u/%u clocks adjust %+.0f us uncertainty %.0f us" % (
                algorithm.wanted, len(algorithm.responses),
                algorithm.adjustment,
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

            print()

            # assert last_wanted <= algorithm.wanted
            last_wanted = algorithm.wanted

            #assert algorithm.adjustment < 1E-3
