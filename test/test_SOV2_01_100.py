#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
"""

import sys
sys.path.append('../libim7/')
import numpy as np
trace = True
flags = [True, True, True, True]

if trace:
    import matplotlib.pyplot as plt
    # Figure1 : vx, 2:vy, 3:vz
    f1 = plt.figure(1)
    f2 = plt.figure(2)
    f3 = plt.figure(3)
    f1.suptitle('$V_x$')
    f2.suptitle('$V_y$')
    f3.suptitle('$V_z$')
    Ax = [f.add_subplot(221) for f in (f1,f2,f3)]
    share = lambda ind: {'sharex': Ax[ind-1], 'sharey':Ax[ind-1]}
    
nx, ny = 118, 78
d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'lower'}
# libim7
if flags[0]:
    import libim7 as im7
    buf1, att1 = im7.readim7('SOV2_01_100_davis.VC7')
    dx = {'extent':(buf1.x[0],buf1.x[-1],buf1.y[0],buf1.y[-1])}
    dx.update(d)
    if trace:
        Ax[0].imshow(buf1.vx.T, **dx)
        Ax[1].imshow(buf1.vy.T, **dx)
        Ax[2].imshow(buf1.vz.T, **dx)
        Ax[0].text(.02, .02, 'VC7', transform=Ax[0].transAxes)

# txt: comma to dot
if flags[1]:
    f = file('SOV2_01_100_davis.txt', 'r')
    f.readline(); string = f.read(); f.close(); string = string.replace(',', '.')
    buf2 = np.fromstring(string, sep='\t').reshape((ny,nx,5))
    x, y = buf2[:,:,0][0,:], buf2[:,:,1][:,0]
    dx = {'extent':(x[0],x[-1],y[0],y[-1])}
    dx.update(d)
    if trace:
        ax=f1.add_subplot(222, **share(1))
        ax.imshow(buf2[:,:,2], **dx)
        ax.text(.02, .02, 'txt', transform=ax.transAxes)
        f2.add_subplot(222, **share(2)).imshow(buf2[:,:,3], **dx)
        f3.add_subplot(222, **share(3)).imshow(buf2[:,:,4], **dx)

# dat
if flags[2]:
    buf3 = np.loadtxt('SOV2_01_100_davis.dat', delimiter=' ', skiprows=3)
    buf3 = buf3.reshape((ny,nx,6))
    x, y = buf3[:,:,0][0,:], buf3[:,:,1][:,0]
    dx = {'extent':(x[0],x[-1],y[0],y[-1])}
    dx.update(d)
    if trace:
        f1.add_subplot(223, **share(1))
        ax.imshow(buf3[:,:,3], **dx)
        ax.text(.02, .02, 'dat', transform=ax.transAxes)
        f2.add_subplot(223, **share(2)).imshow(buf3[:,:,4], **dx)
        f3.add_subplot(223, **share(3)).imshow(buf3[:,:,5], **dx)
    
# mat
if flags[3]:
    import scipy.io as io
    buf4 = io.loadmat('SOV2_01_100_pivmat.mat', matlab_compatible=True)['v']
    # Troubles with incorrect x and y vectors...
    if trace:
        f1.add_subplot(224)
        ax.imshow(buf4['vx'][0,0].T[::-1,:], **d)
        ax.text(.02, .02, 'mat', transform=ax.transAxes)
        f2.add_subplot(224).imshow(-buf4['vy'][0,0].T[::-1,:], **d)
        f3.add_subplot(224).imshow(buf4['vz'][0,0].T[::-1,:], **d)

if trace:
    for ind,fig in enumerate((f1,f2,f3)):
        lAx = fig.get_axes()
#        for ax in lAx[1:]:
#            ax.get_images()[0].set_clim(lAx[0].get_images()[0].get_clim())
        plt.savefig(__file__.replace('.py', '%d.pdf' % ind))
    plt.show()
