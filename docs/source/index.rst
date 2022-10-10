.. Vad är klockan documentation master file, created by
   sphinx-quickstart on Tue Oct  4 11:22:01 2022.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to "vad är klockan?"
============================

"Vad är klockan?"  is Swedish for "what time is it?"

In order to be able to communicate securely on the internet, it is
important to be able to answer the question "what time is it?".  With
time we mean both time of day and date.  Without correct time there
are many several security-critical protocols that either completely
stop working or that lose much of their security.

* DNSSEC, which is needed to look up names on the internet, will not
  work at all if the clock in a client is off by more than five
  minutes.  It's possible to fall back to plain DNS without security,
  but that opens up the client to man in the middle attacks: someone
  who can monitor and affect the communication between client and
  server can force DNS to give wrong answers for domain name name
  lookups.  You might think that you are talking to "example.com",
  while you're actually talking to someone completely different who is
  impersonating that server.

* TLS (formerly known as SSL) needs correct time to verify the
  validity of the certificates used.  TLS is the base for many other
  protocols used on the Internet.

* HTTPS is used to secure the WWW and is based on TLS.  Without secure
  TLS, you can't get secure HTTPS either.  It's often possible to fall
  back to plain HTTP without security, but they are susceptible to man
  in the middle attacks again.

* The same applies to protocols such as SMTPS, IMAPS and POP3S used
  for e-mail; all of them are based on TLS.  There are insecure
  variants, plain SMTP, IMAP and POP3, but once agan they are
  susceptible to man in the middle attacks again.

* Time can also be important for the application itself. It's not good
  if the computer that's supposed unlock a door between 07:00 and
  17:00 on weekdays get the time wrong so the door is open in the
  middle of the night or on a weekend.

Many internet-connected devices do not know the time when they start
up up.  Some units are designed that way, to keep manufacturing costs
down, they have no battery.  When they lose power, they forget what
time it is.  Even devices with a battery can forget what time it is is
when outing a flat battery.  Other devices have a "shipping mode"
where they to save battery during shipping and won't start their
internal clock until they are powered up the first time.  This means
that the first time they start up they will not know what time it is.

The goal of the project "vad är klockan" is to let an
internet-connected device get time securely during boot.  To do this
the project is split into two parts.  The fist part is about
priciples, to find a method to do this.  The second part is to write a
library to actually implement this, a library which should be useful
for as many people as possible.

Method
======

The method can be conceptually divided into two parts:

* To securely get an answer to the question "what time is it?" by
  talking to a time server of some kind. We have chosen a protocol
  called "Roughtime" which has been submitted as an `IETF draft
  <https://datatracker.ietf.org/doc/html/draft-ietf-ntp-roughtime>`_.

* Even if you can query a time server, it is not certain that it
  actually gives the correct time.  A time server might be broken and
  accidentally give incorrect time.  A time server may also have been
  taken over by someone else who deliberatelly has made it respond
  with the wrong time.  To be reasonably sure that the time once gets
  is correct, on one should query multiple time servers and make a
  judgement based on the reeceived responses.  For example, query a
  dozen servers and require that a majority of the responses agree.
  To do this we have based our algorithm on the "selection and
  clustering algorithm" as described in `RFC5906
  <https://datatracker.ietf.org/doc/html/rfc5905>`_.  (Note, please be
  aware that the implementation in RFC5905 is incorrect, see the
  errata for a correct implementation).

By separating the protocol for talking to time servers from the the
algorithm for choosing which ones to trust it is relative easy to
replace any of the parts in the future.  Since Roughtime is currently
a draft, there is a high probability that it will receive incompatible
changes before it becoming a proper RFC.  It might also be that some
other protocol to securely query time servers will become the de facto
standard in future.

Implementation
==============

For the implementation part of, we have chosen to make two implementations.

* An implementation in Python with the goal to be easy to understand.
  We has based this an existing implementations of the Roughtime
  protocol called "pyroughtime" by Marcus Dansarie.  The algorithm for
  choosing which time servers to trust has been written from scratch.
  One of the advantages of Python is that it has been relative easy to
  simulate and visualize the algorithms used.

* An implementation in C whose goal is to be small, compact, safe and
  reusable on as many internet-connected devices as possible.  Most
  platforms, ranging from small IoT devices based on Arduino,
  Raspberry Pi, or a real computer running Linux, Windows or MacOS can
  run code written in C.  The support for The Roughtime protocol is
  based on an implementation called "vroughtime" by Oscar Reparaz. The
  algorithm to select which ones time-servers to trust is a port of
  the algorithm in Python code above.

Reference platforms
===================

These implementations then need some glue code to run the target
platform one chooses.  We have chosen two target platforms to begin
with, but it should be easy to port the code to others platforms.

* Development of both the Python and C implementations has taken place
  on a computer running Linux.  Most of the code is cross-platform,
  but some platform-specific glue code was needed. The same code ought
  to work on Windows and Unix-based operating systems such as MacOS
  but has not been tested on those platforms so far.

* Additionally, we have chosen to port the implementation to a small
  one ESP32 based device with WiFi to show it also works on smaller
  IoT devices.  The ESP32-based platform can run a stripped down
  version of the Python implementation in a MicroPython environment
  and the C implementation runs in an Arduino environment.  That ESP32
  device is called "TTGO" but the code should run on most ESP32
  devices and possibly also on ESP8266 devices.

.. image:: ttgo.jpg
   :width: 600
   :height: 400
   :alt: TTGO ESP32

Python Implementation Details
=============================

The existing project "pyroughtime" supports both the draft 06 version
and older versions of Roughtime.

Python on Linux
---------------

The Python 3 implementation for Linux can be found in examples/python.

This version should run on most modern Linux distributions with Python 3.

MicroPython on ESP32
--------------------

The Python implementation for TTGO ESP32 can be found in examples/micropython-esp32-ttgo.

This is mostly a "toy implementation" to show that it is possible to
run the Python code on a tiny platform.  In production it would be
better to port the C implementation of "vad är klockan" to the
Micropython environment and create a Python wrapper for it.

MicroPython on the TTGO ESP32 has very little free memory available
and alse differs quite a bit much from mainline Python.  In order to
accommodate this "pyroughtime" has had to be slimmed down and modified
considerably.

MicroPython for ESP32 has been extended with support for those
encryption primitives needed for Roughtime (ed25519, sha512) and to be
able to set the clock from Python code (settimeofday).

C Implementation Details
========================

"vroughtime" did not initially support draft 06 so the code has
modified to support both draft 06 and older versions of Roughtime.
The same code is used both on Linux and on ESP32.

C on Linux
----------

The C implementation for Linux can be found in examples/C.

Some glue code needed to be written to run on Linux and for UDP
communication via BSD sockets API.  The code supports both IPv4 and
IPv6.

C on ESP32
----------

The C implementation for Linux can be found in examples/esp32.

Some sticky code needed to be written to use WiFi and UDP on the ESP32.
The code so far only supports IPv4.

The Roughtime protocol
======================

Since the Roughtime protocol is a "draft" there are not that many
usable Roughtime time-servers out the.  There is only a single server
implementation that fully supports `IETF draft version 06 of Roughtime
<https://tools.ietf.org/html/draft-ietf-ntp-roughtime-06>`_.  This
implementation, written in C, is made by Marcus Dansarie.

Netnod has chosen to set up several Roughtime servers with this
implementation (sth1.roughtime.netnod.se and
sth2.roughtime.netnod.se).  Netnod has also set up several Roughtime
servers that with incorrect time (falseticker.roughtime.netnod.se),
these servers should never be used in production, but are useful for
validating the algorithms used to discard bad Roughtime servers.

Implementation
==============

The project "What time is it?" has been carried out during 2022 by
Calle Lindkvist (C implementation), Filip Eriksson (Python
implementation) under the supervision of Christer Weinigel (C/Python
and server infrastructure at Netnod as well as packaging and
documentation).

API
===

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   python_overlap
   pyroughtime
   c_overlap
   vrt
   tweetnacl
   bakgrund

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
