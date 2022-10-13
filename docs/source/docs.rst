Documentation
=============

The documentation for "vad är klockan" is generated using Sphinx.  The
Python API documentation are generated from docstrings in the code.
The C API documenation is generated from Doxygen comments in the code
and pulled into Sphinx using Breathe.  Some of the documentation is
written i Jupyter Notebook which pulled into sphinx using nbsphinx.

.. highlight:: bash

    sudo apt install doxygen

.. highlight:: bash

     pip3 install -r requirements.txt

To build the HTML documentation, enter the "docs" directory and run
the following command:

.. highlight:: bash

    make clean html



