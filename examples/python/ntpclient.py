import machine, time, math, network, utime

use_display = False

if use_display:
    import display

import network
sta = network.WLAN(network.STA_IF)
if not sta.active():
    sta.active(True)

    sta.scan()
    sta.connect('wx', 'paltkoma63')

if use_display:
    print(globals())
    if 'tft' not in globals():
        print("Initializing display")
        tft = display.TFT()
        # tft.deinit()
        tft.init(tft.ST7789,bgr=False,rot=tft.LANDSCAPE, miso=17,backl_pin=4,backl_on=1, mosi=19, clk=18, cs=5, dc=16)
        tft.setwin(40,52,320,240)

while not sta.isconnected():
    pass

print(sta.ifconfig())

import socket
import binascii
import struct

if 1:
    fmt = '>LLLLQQQQ'
    query = struct.pack(fmt, 0x23000000, 0, 0, 0, 0, 0, 0, 0)
    print(binascii.hexlify(query))

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
    s.sendto(query, ('194.58.202.20', 123))
    resp = s.recv(1024)
    assert len(resp) == 48
    print(binascii.hexlify(resp))

    v = struct.unpack(fmt, resp)
    print(v)
    print("ver %u" % ((v[0] >> 27) & 7))
    print("mode %u" % ((v[0] >> 24) & 7))
    print("stratum %u" % ((v[0] >> 16) & 255))
    print("poll %u" % ((v[0] >> 8) & 255))
    print("precision %u" % (v[0] & 255))
    print("root delay %u" % (v[1]))
    print("root dispersion %u" % (v[2]))
    print("reference id 0x%04x" % (v[3]))

    EPOCH = 2208988800

    import time
    tm = time.gmtime(int(v[6] / 2**32 - EPOCH))
    print(tm)

    text = "%04u-%02u-%02u %02u:%02u:%02u" % (
        tm[0], tm[1], tm[2], tm[3], tm[4], tm[5])

    if use_display:
        tft.text(120-int(tft.textWidth(text)/2),100-int(tft.fontSize()[1]/2),text,0x808080)
