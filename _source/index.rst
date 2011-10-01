.. libim7 documentation master file, created by
   sphinx-quickstart on Sat Oct  1 09:42:14 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to libim7's documentation!
==================================

The libim7 project intends to provide a simple pythonic interface to read Particle Image Velocimetry (PIV) image and vector fields files created by LaVision Davis software.

It bases on ctypes to build an object-oriented interface to the C library provided by LaVision.

Build and install
-----------------

Reference API
-------------

.. automodule:: libim7
   :show-inheritance:

   .. autofunction:: libim7.readim7

   .. autoclass:: libim7.Buffer
      :members: get_frame, delete

   .. autoclass:: libim7.AttributeList
   
   .. autofunction:: libim7.save_as_pivmat
      
Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

