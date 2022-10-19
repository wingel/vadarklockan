Python Implementation
=====================

The existing project "pyroughtime" supports both the draft 06 version
and older versions of Roughtime.

The reference Python implementation can be found in the directory
"src/python".

Python on Linux
---------------

The Python 3 implementation for Linux can be found in examples/python.

This version should run on most modern Linux distributions with Python 3.

.. todo:: Add instructions on how to run the Python code on Linux.
          pip install commands for requirements etc

Unit testing
------------

Some of the python classes contain unit tests.  The tests are based on
the Python "unittest" framework.  To perform those tests run:

.. highlight:: bash

::

    cd examples/python
    python3 -m unittest test*.py

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

.. todo:: Add instructions on how to run the Python code on ESP32
          including on where to get a modified MicroPython and pointer
          and how to use "run.py"

Python API
----------

.. toctree::
   :maxdepth: 2

   python_overlap
   pyroughtime
