import ctypes as ct
import numpy.ctypeslib as nct

#mylib = ct.cdll.LoadLibrary("./libReadIM7.so")
mylib = nct.load_library("libim7", ".")

char16 = ct.c_char*16
word = ct.c_ushort
floatarr = nct.ndpointer(dtype=ct.c_float, flags='C_CONTIGUOUS')
wordarr = nct.ndpointer(dtype=ct.c_ushort, flags='C_CONTIGUOUS')
class BufferScale(ct.Structure):
    _fields_ = [("factor", ct.c_float), ("offset", ct.c_float),
                ("description", char16), ("unit", char16)]
    _fnames_ = map(lambda x: x[0], _fields_)
    
    def _get_fdict(self):
        return dict(map(lambda x: (x[0], self.__getattribute__(x[0])), self._fields_))
    _fdict_ = property(_get_fdict)
    
    def __repr__(self):
        tmp = self.description + " (%s)" % self.__class__
        tmp += ":\n\t%.3f*n+%.3f" %(self.factor, self.offset)
        if self.unit!="":
            tmp += " (%s)" % self.unit
        return tmp
    
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
        
    def get_array(self):
        # while nct.prep_pointer is not available on debian unstable
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

class AttributeList(ct.Structure):
    pass
AttributeList._fields_ = [("name", ct.c_char_p), ("value", ct.c_char_p), \
    ("next", ct.POINTER(AttributeList))]

(IMREAD_ERR_NO, IMREAD_ERR_FILEOPEN, IMREAD_ERR_HEADER, IMREAD_ERR_FORMAT, \
    IMREAD_ERR_DATA, IMREAD_ERR_MEMORY) = range(6)

def imread_errcheck(retval, func, *args):
    if func.__name__!="ReadIM7":
        raise ValueError(u"Wrong function passed: %s." % func.__name__)
    if retval==IMREAD_ERR_FILEOPEN:
        raise IOError(u"Can't open file %s." % args[0])
    elif retval==IMREAD_ERR_HEADER:
        raise ValueError(u"Incorrect header in file %s." % args[0])
    elif retval==IMREAD_ERR_FORMAT:
        raise IOError(u"Incorrect format in file %s." % args[0])
    elif retval==IMREAD_ERR_DATA:
        raise IOError(u"Error while reading data in %s." % args[0])
    elif retval==IMREAD_ERR_MEMORY:
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
    return mybuffer, mylist

if __name__=='__main__':
    scale = BufferScale(1., 0., "blabla", "mm")
    scale.setbufferscale(2., 10, description="aze", unit='inches')
    buf, att = readim7("test/B00001.VC7")
    data = buf.get_array()
    ny = 128
    Nitype = 10
    # plot
    import matplotlib.pyplot as plt
    plt.figure()
    for n in xrange(Nitype/2-1):
        plt.subplot(3,4,5+n)
        plt.imshow(data[(2*n+1)*ny:(2*n+2)*ny, :])
        plt.colorbar()
        if n==0:
            plt.ylabel('Vx')
        plt.subplot(3,4,9+n)
        plt.imshow(data[(2*n+2)*ny:(2*n+3)*ny, :])
        plt.colorbar()
        if n==0:
            plt.ylabel('Vy')
    plt.subplot(3,4,2)
    plt.imshow(data[0:ny, :])
    plt.ylabel('Choice')
    plt.colorbar()
    plt.subplot(3,4,4)
    plt.imshow(data[(Nitype-1)*ny:, :])
    plt.xlabel('Peak ratio')
    plt.colorbar()
    plt.show()
