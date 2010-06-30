#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
"""

import sys
sys.path.append('..')
import numpy as np
trace = True
flags = [True, True, True, True]

if trace:
    import matplotlib.pyplot as plt
    # Figure1 : vx, 2:vy, 3:vz
    f1,f2,f3 = plt.figure(1),plt.figure(2),plt.figure(3)
    Ax = [f.add_subplot(221) for f in (f1,f2,f3)]
    share = lambda ind: {'sharex': Ax[ind-1], 'sharey':Ax[ind-1]}
    
nx, ny = 118, 78
d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'upper'}
# libim7
if flags[0]:
    import libim7 as im7
    buf1, att1 = im7.readim7('SOV2_01_100_davis.VC7')
    dx = {}#'extent':(buf1.x[0],buf1.x[-1],buf1.y[0],buf1.y[-1])}
    dx.update(d)
    if trace:
        Ax[0].imshow(buf1.vx, **dx)
        Ax[1].imshow(buf1.vy, **dx)
        Ax[2].imshow(buf1.vz, **dx)

# txt: comma to dot
if flags[1]:
    f = file('SOV2_01_100_davis.txt', 'r')
    f.readline(); string = f.read(); f.close(); string = string.replace(',', '.')
    buf2 = np.fromstring(string, sep='\t').reshape((ny,nx,5))
    if trace:
        f1.add_subplot(222, **share(1)).imshow(buf2[:,:,2], **d)
        f2.add_subplot(222, **share(2)).imshow(buf2[:,:,3], **d)
        f3.add_subplot(222, **share(3)).imshow(buf2[:,:,4], **d)

# dat
if flags[2]:
    buf3 = np.loadtxt('SOV2_01_100_davis.dat', delimiter=' ', skiprows=3)
    buf3 = buf3.reshape((ny,nx,6))
    if trace:
        f1.add_subplot(223, **share(1)).imshow(buf3[:,:,3], **d)
        f2.add_subplot(223, **share(2)).imshow(buf3[:,:,4], **d)
        f3.add_subplot(223, **share(3)).imshow(buf3[:,:,5], **d)
    
# mat
if flags[3]:
    import scipy.io as io
    buf4 = io.loadmat('SOV2_01_100_pivmat.mat', squeeze_me=True)['v']
    if trace:
        f1.add_subplot(224, **share(1)).imshow(buf4.vx.T[::-1,:], **d)
        f2.add_subplot(224, **share(2)).imshow(-buf4.vy.T[::-1,:], **d)
        f3.add_subplot(224, **share(3)).imshow(buf4.vz.T[::-1,:], **d)

if trace:
    plt.show()
