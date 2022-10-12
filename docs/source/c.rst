C Implementation
================

"vroughtime" did not initially support draft 06 so the code has
modified to support both draft 06 and older versions of Roughtime.
The same code is used both on Linux and on ESP32.

C on Linux
----------

The C implementation for Linux can be found in examples/C.

Some glue code needed to be written to run on Linux and for UDP
communication via BSD sockets API.  The code supports both IPv4 and
IPv6.

.. todo:: Add instructions on how to compile and run the C code on Linux

C on ESP32
----------

The C implementation for Linux can be found in examples/esp32.

Some sticky code needed to be written to use WiFi and UDP on the ESP32.
The code so far only supports IPv4.

.. todo:: Add instructions on how to compile and run the C code on ESP32

C API
-----

.. toctree::
   :maxdepth: 2

   c_overlap
   vrt
   tweetnacl
