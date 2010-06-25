#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
"""

import sys
sys.path.append('..')
import numpy as np
import scipy.io as io
import libim7 as im7
import matplotlib.pyplot as plt

# Figure1 : vx, 2:vy, 3:vz
f1,f2,f3 = plt.figure(1),plt.figure(2),plt.figure(3)
nx, ny = 118, 78

# libim7
if True:
    buf1, att1 = im7.readim7('SOV2_01_100_davis.VC7')
    d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'upper', 
        'extent':(buf1.x[0],buf1.x[-1],buf1.y[0],buf1.y[-1])}
    f1.add_subplot(221).imshow(buf1.vx, **d)
    f2.add_subplot(221).imshow(buf1.vy, **d)
    f3.add_subplot(221).imshow(buf1.vz, **d)

# txt: comma to dot
if True:
    f = file('SOV2_01_100_davis.txt', 'r')
    f.readline(); string = f.read(); f.close(); string = string.replace(',', '.')
    buf2 = np.fromstring(string, sep='\t').reshape((ny,nx,5))
    d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'upper'}
    f1.add_subplot(222).imshow(buf2[:,:,2], **d)
    f2.add_subplot(222).imshow(buf2[:,:,3], **d)
    f3.add_subplot(222).imshow(buf2[:,:,4], **d)

# dat
if True:
    buf3 = np.loadtxt('SOV2_01_100_davis.dat', delimiter=' ', skiprows=3)
    buf3 = buf3.reshape((ny,nx,6))
    d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'upper'}
    f1.add_subplot(223).imshow(buf3[:,:,3], **d)
    f2.add_subplot(223).imshow(buf3[:,:,4], **d)
    f3.add_subplot(223).imshow(buf3[:,:,5], **d)
    
# mat
if True:
    buf4 = io.loadmat('SOV2_01_100_pivmat.mat', squeeze_me=True)['v']
    d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'lower'}
    f1.add_subplot(224).imshow(buf4.vx.T, **d)
    f2.add_subplot(224).imshow(-buf4.vy.T, **d)
    f3.add_subplot(224).imshow(buf4.vz.T, **d)

plt.show()
