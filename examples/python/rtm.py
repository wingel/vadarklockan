import machine, time, network
import binascii

use_display = False

if use_display:
    import display

sta = network.WLAN(network.STA_IF)
if not sta.active():
    sta.active(True)

    sta.scan()
    # sta.connect('wx', 'paltkoma63')
    sta.connect('Test123', 'gv28985v')

while not sta.isconnected():
    pass

print(sta.ifconfig())

time.sleep(3)

import rt

def main():
    host = 'sth1.roughtime.netnod.se'
    port = 2002
    pubkey = binascii.a2b_base64('9l1JN4HakGnG44yyqyNNCb0HN0XfsysBbnl/kbZoZDc=')
    newver = True

    print("create client")

    cl = rt.RoughtimeClient()

    print("query")

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
    print('Merkle tree path length: %d' % repl['pathlen'])

if __name__ == '__main__':
    main()
