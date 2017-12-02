#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
Files provided by Colin Brosseau (B0003.*->test_IMX.*)
"""

import sys
sys.path.append('../libim7')
import numpy as np
import libim7 as im7

trace = True
lExt = (".imx", ".IM7")
if trace:
    import matplotlib.pyplot as plt
    # Figure1 : vx, 2:vy, 3:vz
    f1 = plt.figure(1)
    ax0 = f1.add_subplot(211)
    ax1 = f1.add_subplot(212)
    lAxs = (ax0, ax1)
    
d = {
    'interpolation':'nearest', 
#    'vmin':-10, 'vmax':10, 
    'origin':'upper', 
    'aspect':'auto'}

lBuf = []
for ind, ext in enumerate(lExt[::-1]):
    fname = __file__.replace(".py", ext)
    buf1, att1 = im7.readim7(fname)
    lBuf.append(buf1)
    dx = {'extent':(buf1.x[0], buf1.x[-1], buf1.y[0], buf1.y[-1])}
    dx.update(d)
    if trace:
        im = lAxs[ind].imshow(buf1.get_frame(0), **dx)
        lAxs[ind].text(.02, .02, fname, transform=lAxs[ind].transAxes, color='c')
        lAxs[ind].set_xlabel(buf1.scaleX.as_label())
        lAxs[ind].set_ylabel(buf1.scaleY.as_label())
        cb = plt.colorbar(im, ax = lAxs[ind])
        cb.set_label(buf1.scaleI.as_label())

if trace:
    f1.tight_layout()
    plt.savefig(__file__.replace('.py', '.pdf'))
    #plt.close('all')
    plt.show()
