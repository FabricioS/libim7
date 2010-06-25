#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
This project intends to provide a simple pythonic interface to read 
Particle Image Velocimetry (PIV) image and vector fields files created 
by LaVision Davis software.
It bases on ctypes to build an object-oriented interface to their C library.
"""

import numpy as np
import ctypes as ct
import numpy.ctypeslib as nct

import os
try:
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "_im7.so")
except NameError:
    path = "./_im7.so"
mylib = ct.cdll.LoadLibrary(path)
char16 = ct.c_char*16
word = ct.c_ushort
byte = ct.c_ubyte

# Error code returned by ReadIM7 C function
ImErr = {
    'IMREAD_ERR_NO':0,     'IMREAD_ERR_FILEOPEN':1, \
    'IMREAD_ERR_HEADER':2, 'IMREAD_ERR_FORMAT':3, \
    'IMREAD_ERR_DATA':4,   'IMREAD_ERR_MEMORY':5}

# Code for the buffer format
Formats = {
    'Formats_NOTUSED':-1, 'FormatsMEMPACKWORD':-2, \
    'FormatsFLOAT': -3,   'FormatsWORD':-4, \
    'FormatsDOUBLE':-5,   'FormatsFLOAT_VALID':-6, \
    'FormatsIMAGE':0,     'FormatsVECTOR_2D_EXTENDED':1, \
    'FormatsVECTOR_2D':2, 'FormatsVECTOR_2D_EXTENDED_PEAK':3,	\
	'FormatsVECTOR_3D':4, 'FormatsVECTOR_3D_EXTENDED_PEAK':5,	\
    'FormatsCOLOR':-10,   'FormatsRGB_MATRIX':-10, \
    'FormatsRGB_32':-11}
    
class BufferScale(ct.Structure):
    " Linear scaling tranform. "
    _fields_ = [("factor", ct.c_float), ("offset", ct.c_float),
                ("description", char16), ("unit", char16)]
    _fnames_ = map(lambda x: x[0], _fields_)
    
    def _get_fdict(self):
        return dict( \
            map(lambda x: (x[0], self.__getattribute__(x[0])), self._fields_))
    _fdict_ = property(_get_fdict)
    
    def __repr__(self):
        tmp = self.description + " (%s)" % self.__class__
        tmp += ":\n\t(%.3f)*n+(%.3f)" %(self.factor, self.offset)
        if self.unit!="":
            tmp += " (%s)" % self.unit
        return tmp

    def __call__(self, vector, grid):
        if (isinstance(vector,int)):
            vector = np.arange(.5, vector)
        return vector*self.factor*grid+self.offset
    
    def setbufferscale(self, *args, **kwargs):
        latt = [tmp for tmp in self._fnames_ if not(tmp in kwargs.keys())]
        dic = self._fdict_.copy()
        dic.update(kwargs)
        if len(args)<=len(latt):
            for ind,val in enumerate(args):
                dic[latt[ind]] = val
        else:
            print args, kwargs
            raise IOError(u"Too many arguments in _setbufferscale.")
        mylib.SetBufferScale(ct.byref(self), dic["factor"], dic["offset"], \
            ct.c_char_p(dic["description"]), ct.c_char_p(dic["unit"]))

class ImageHeader7(ct.Structure):
    _fields_ = [ \
        ("version", ct.c_short), \
        ("pack_type", ct.c_short), \
        ("buffer_format", ct.c_short), \
        ("isSparse", ct.c_short), \
        ("sizeX", ct.c_int), \
        ("sizeY", ct.c_int), \
        ("sizeZ", ct.c_int), \
        ("sizeF", ct.c_int), \
        ("scalarN", ct.c_short),  \
        ("vector_grid", ct.c_short),  \
        ("extraFlags", ct.c_short), \
        ("reserved", ct.c_byte*(256-30))]

class _Data(ct.Union):
    _fields_ = [("floatArray", ct.POINTER(ct.c_float)), \
                ("wordArray",  ct.POINTER(ct.c_ushort))]

class Buffer(ct.Structure):
    _anonymous_ = ("array",)
    _fields_ = [("isFloat", ct.c_int), \
        ("nx", ct.c_int), ("ny", ct.c_int), \
        ("nz", ct.c_int), ("nf", ct.c_int), \
        ("totalLines", ct.c_int), \
        ("vectorGrid", ct.c_int), \
        ("image_sub_type", ct.c_int), \
        ("array", _Data), \
        ("scaleX", BufferScale), \
        ("scaleY", BufferScale), \
        ("scaleI", BufferScale)]

    def read_header(self):
        f = file(self.file, 'rb')
        tmp = f.read(ct.sizeof(ImageHeader7))
        f.close()
        self.header = ImageHeader7()
        ct.memmove(ct.addressof(self.header), ct.c_char_p(tmp), \
            ct.sizeof(ImageHeader7))
    
    def get_array(self):
        """ 
        Prepare data pointer to behave as a numpy array.
        rem: Should become obsolete with nct.prep_pointer.
        """
        try:
            self.array.__array_interface__
            return np.array(self.array, copy=False)
        except AttributeError:
            pass
            
        if self.isFloat==1:
            arr = self.floatArray
        else:
            arr = self.wordArray
        # Constructing array interface
        arr.__array_interface__ = {'version': 3, \
            'shape':(self.totalLines, self.nx), \
            'data': (ct.addressof(arr.contents), False),\
            'typestr' : nct._dtype(type(arr.contents)).str}
        return np.array(arr, copy=False)

    def __getattr__(self, key):
        if key=='header':
            self.read_header()
            return self.header
        elif key=='blocks':
            self.get_blocks()
            return self.blocks
        elif key in ('x', 'y', 'z'):
            self.get_positions()
            return self.__dict__[key]
        elif key in ('vx', 'vy', 'vz', 'vmag'):
            self.get_components()
            return self.__dict__[key]
        else:
            raise AttributeError(u"Does not have %s atribute" % key)
    
    def get_positions(self):
        self.x = self.scaleX(self.header.sizeX, self.vectorGrid)   
        self.y = self.scaleY(self.header.sizeY, self.vectorGrid)
        self.z = np.arange(self.header.sizez)
        
    def get_blocks(self):
        " Transforms the concatenated blocks into arrays."
        h = self.header
        arr = self.get_array()       
        if h.buffer_format==Formats['FormatsIMAGE']:
            self.blocks = arr.reshape((h.sizeZ*h.sizeF, h.sizeY, h.sizeX))
        elif h.buffer_format>=1 and h.buffer_format<=5:
            nblocks = (9, 2, 10, 3, 14)
            nblocks = nblocks[h.buffer_format-1]
            self.blocks = arr.reshape((nblocks, h.sizeY, h.sizeX))
        else:
            raise TypeError(u"Can't get blocks from this buffer format.")
    
    def get_components(self):
        """
        Extract the velocity components from the various blocks stored in
        Davis files according to values in the header.
        """
        h = self.header
        b = self.blocks     
        choice = (np.array(b[0,:,:], dtype=int)-1)%4
        if h.buffer_format==Formats['FormatsVECTOR_2D']:
            vx = b[0,:,:]
            vy = b[1,:,:]
            vz = np.zeros_like(vx)
        elif h.buffer_format==Formats['FormatsVECTOR_2D_EXTENDED']:
            vx = np.choose(choice, (b[1,:,:], b[3,:,:], b[5,:,:], b[7,:,:], b[7,:,:], b[7,:,:]))
            vy = np.choose(choice, (b[2,:,:], b[4,:,:], b[6,:,:], b[8,:,:], b[8,:,:], b[8,:,:]))
            vz = np.zeros_like(vx)
        elif  h.buffer_format==Formats['FormatsVECTOR_2D_EXTENDED_PEAK']:
            vx = np.choose(choice, (b[1,:,:], b[3,:,:], b[5,:,:], b[7,:,:], b[7,:,:], b[7,:,:]))
            vy = np.choose(choice, (b[2,:,:], b[4,:,:], b[6,:,:], b[8,:,:], b[8,:,:], b[8,:,:]))
            vz = np.zeros_like(vx)
            self.peak = b[9,:,:]
        elif h.buffer_format==Formats['FormatsVECTOR_3D']:
            vx = b[0,:,:]
            vy = b[1,:,:]
            vz = b[2,:,:]
        elif  h.buffer_format==Formats['FormatsVECTOR_3D_EXTENDED_PEAK']:
            vx = np.choose(choice, (b[1,:,:], b[4,:,:], b[7,:,:], b[10,:,:], b[10,:,:], b[10,:,:]))
            vy = np.choose(choice, (b[2,:,:], b[5,:,:], b[8,:,:], b[11,:,:], b[11,:,:], b[11,:,:]))
            vz = np.choose(choice, (b[3,:,:], b[6,:,:], b[9,:,:], b[12,:,:], b[12,:,:], b[12,:,:]))
            self.peak = b[13,:,:]
        else:
            raise TypeError(u"Object does not have a vector field format.")
#        if self.scaleI.factor<0:
#            self.scaleI.factor *= -1
#            self.scaleI.offset *= -1
        self.vx = self.scaleI(vx, 1.)
        self.vy = self.scaleI(vy, 1.)
        self.vz = self.scaleI(vz, 1.)
        self.vmag = np.sqrt(self.vx**2+self.vy**2+self.vz**2)
    
    def filter(self, fun=None, arrays=[]):
        """
        Mask vector fields with the result of the application of function fun
        taking a buffer as input argument. Returns velocity components
        as masked arrays, and may apply same process to other array arguments.
        """
        idx = fun(self)
        ff = lambda m: np.ma.array(m, mask=idx)
        lArrays = [self.vx, self.vy, self.vz]
        for tmp in arrays:
            if tmp.shape == self.vx.shape:
                lArrays.append(tmp)
            else:
                raise ValueError(u'Wrong shape for additional argument.')
        return map(ff, lArrays)
    
    def quiver_xyplane(self, ax=None, sep=1):
        ax = quiver_3d(self.x, self.y, self.vx, self.vy, self.vz, ax, sep)
    
            
class AttributeList(ct.Structure):
    pass
AttributeList._fields_ = [("name", ct.c_char_p), ("value", ct.c_char_p), \
    ("next", ct.POINTER(AttributeList))]
def AttList_to_dict(att):
    dic = {}
    while att!=0:
        try:
            dic[att.name] = att.value
            att = att.next
        except AttributeError:
            break
    return dic
AttributeList.todict = AttList_to_dict

def imread_errcheck(retval, func, args):
    if func.__name__!="ReadIM7":
        raise ValueError(u"Wrong function passed: %s." % func.__name__)
    if retval==ImErr['IMREAD_ERR_FILEOPEN']:
        raise IOError(u"Can't open file %s." % args[0])
    elif retval==ImErr['IMREAD_ERR_HEADER']:
        raise ValueError(u"Incorrect header in file %s." % args[0])
    elif retval==ImErr['IMREAD_ERR_FORMAT']:
        raise IOError(u"Incorrect format in file %s." % args[0])
    elif retval==ImErr['IMREAD_ERR_DATA']:
        raise ValueError(u"Error while reading data in %s." % args[0])
    elif retval==ImErr['IMREAD_ERR_MEMORY']:
        raise MemoryError(u"Out of memory while reading %s." % args[0])
    else:
        pass

mylib.SetBufferScale.argtypes = [ct.POINTER(BufferScale), \
    ct.c_float, ct.c_float, ct.c_char_p, ct.c_char_p]
mylib.SetBufferScale.restype = None
mylib.ReadIM7.argtypes = [ct.c_char_p, ct.POINTER(Buffer), ct.POINTER(AttributeList)]
mylib.ReadIM7.restype = ct.c_int
mylib.ReadIM7.errcheck = imread_errcheck

def readim7(filename):
    mybuffer = Buffer()
    mylist = AttributeList()
    mylib.ReadIM7(ct.c_char_p(filename), ct.byref(mybuffer), ct.byref(mylist))
    mybuffer.file = filename
    return mybuffer, mylist

def show_scalar_field(arr, extent=None, ax=None, colorbar=False):
    import matplotlib.pyplot as plt
    if ax==None:
        ax = plt.figure().add_subplot(111)
    im = ax.imshow(arr, interpolation='nearest', aspect='equal', origin='lower', \
        vmin=arr.min(), vmax=arr.max(), extent=extent)
    if colorbar:
        plt.colorbar(im)
    return ax

def quiver_3d(x,y,vx,vy,vz, ax=None, sep=1):
    import matplotlib.pyplot as plt
    if ax==None:
        ax = plt.figure().add_subplot(111)
    Q = ax.quiver(vy[::sep,::sep], vx[::sep,::sep], \
        vz[::sep,::sep], pivot='mid')
    qk = plt.quiverkey(Q, 0.9, 0.95, 5, r'$5 \frac{m}{s}$',
               labelpos='E',
               coordinates='figure',
               fontproperties={'weight': 'bold'})
    return ax

if __name__=='__main__':
    buf, att = readim7("./test/B00001.VC7")
    
    def myfilter(buf):
        return buf.blocks[0,:,:]!=5
#        V = buf.vmag
#        mean, std = V.mean(), V.std()
#        N = 1.5
#        return np.logical_or(V<mean-N*std, V>mean+N*std)
    
    show_scalar_field(buf.blocks[0,:,:])
    vx, vy, vz, vmag = buf.filter(myfilter, arrays=[buf.vmag,])
    import matplotlib.pyplot as plt
    import matplotlib.cm as cm
    plt.figure()
    plt.quiver(buf.x,buf.y,vx, vy, vmag, cmap=cm.jet)
    plt.colorbar()
    plt.show()
