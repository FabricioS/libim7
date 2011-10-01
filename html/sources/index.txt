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
   For non-Windows OSes, the build of the extension is handled by the DistUtils
   mechanisms::
      
      $ cd libim7
      $ ls
      libim7.py  SConstruct  License.txt  setup.cfg  setup.py  src  test
      $ python setup.py build
      $ ls
      build  libim7.py  SConstruct  License.txt  setup.cfg  setup.py  src  test  _im7.so  
   
   The file named _im7.so (or _im7.dynlib on Mac OSes?) contained the Python
   extension for the code provided by LaVision.
   
   On Windows, one has first to build a shared library file (call it _im7.dll)
   linked against the zlib. The SConstruct file is provided and automates this
   step using the sources in the folder src/::
      
      C:\path\to\libim7\>scons
      C:\path\to\libim7\>python setup.py build
   
   For Python to be able to find the module, you need to set the path 
   (using :py:func:`sys.path` ) or install the library::
      
      # python setup.py install

Usage
-----
   Once build, you can import the module named libim7. The entry point is the 
   function :function:`libim7.readim7` that retrieves the Buffer and the 
   AttributeList objects from a given file. 
   
   .. code-block:: python
      
      >>> import libim7
      >>> buf, att = libim7.readim7('myfile.im7')
    
   All information accessible from LaVision Software is shown in the AttributeList
   
   .. code-block:: python
      
      >>> print(att)

   For an image file, you can get the number of frames and these various frames
   
   .. code-block:: python
   
      >>> print "Number of frames:",buf.nf
      >>> buf.get_frame(0)
   
   For velocity field files (for example from LaVision Davis), the various
   components of the velocity field are retrieved and can be manipulated like
   :py:func:`numpy.ndarray` with:
   
   .. code-block:: python
   
      >>> buf.x
      >>> import matplotlib.pyplot as plt
      >>> plt.imshow(buf.vx, extent=[buf.x[0], buf.x[-1], buf.y[0], buf.y[-1]])
      >>> plt.figtitle(r"$v_x$ (%s)" % buf.scaleI.unit)
      >>> plt.show()

Reference API
-------------

.. automodule:: libim7
   :show-inheritance:

   .. autofunction:: libim7.readim7

   .. autoclass:: libim7.Buffer
      :members: get_frame, delete

   .. autoclass:: libim7.AttributeList
   
   .. autofunction:: libim7.save_as_pivmat
