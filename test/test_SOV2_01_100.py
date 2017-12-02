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
    f1, ax1 = plt.subplots(2, 2, sharex=True, sharey=True, num=1)
    f2, ax2 = plt.subplots(2, 2, sharex=True, sharey=True, num=2)
    f3, ax3 = plt.subplots(2, 2, sharex=True, sharey=True, num=3)

    f1.suptitle('$V_x$')
    f2.suptitle('$V_y$')
    f3.suptitle('$V_z$')
    
nx, ny = 118, 78
d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10,
     'origin':'lower', 'cmap': plt.cm.RdBu}
# libim7
if flags[0]:
    import libim7 as im7
    buf1, att1 = im7.readim7('SOV2_01_100_davis.VC7')
    dx = {'extent':(buf1.x[0],buf1.x[-1],buf1.y[0],buf1.y[-1])}
    dx.update(d)
    if trace:
        ax1[0, 0].imshow(buf1.vx.T, **dx)
        ax2[0, 0].imshow(buf1.vy.T, **dx)
        ax3[0, 0].imshow(buf1.vz.T, **dx)
        ax1[0, 0].set_title('libim7 on .vc7')

# txt: comma to dot
if flags[1]:
    with open('SOV2_01_100_davis.txt', 'r') as f:
        f.readline()
        string = f.read()
    string = string.replace(',', '.')
    buf2 = np.fromstring(string, sep='\t').reshape((ny,nx,5))
    x, y = buf2[:,:,0][0,:], buf2[:,:,1][:,0]
    dx = {'extent':(x[0],x[-1],y[0],y[-1])}
    dx.update(d)
    if trace:
        ax1[0, 1].imshow(buf2[:,:,2], **dx)
        ax2[0, 1].imshow(buf2[:,:,3], **dx)
        ax3[0, 1].imshow(buf2[:,:,4], **dx)
        ax1[0, 1].set_title('np.fromstring on .txt')

# dat
if flags[2]:
    buf3 = np.loadtxt('SOV2_01_100_davis.dat', delimiter=' ', skiprows=3)
    buf3 = buf3.reshape((ny,nx,6))
    x, y = buf3[:,:,0][0,:], buf3[:,:,1][:,0]
    dx = {'extent':(x[0],x[-1],y[0],y[-1])}
    dx.update(d)
    if trace:
        ax1[1, 0].imshow(buf3[:,:,3], **dx)
        ax2[1, 0].imshow(buf3[:,:,4], **dx)
        ax3[1, 0].imshow(buf3[:,:,5], **dx)
        ax1[1, 0].set_title('np.loadtxt on .dat')
    
# mat
if flags[3]:
    import scipy.io as io
    buf4 = io.loadmat('SOV2_01_100_pivmat.mat', matlab_compatible=True)['v']
    # Troubles with incorrect x and y vectors...
    if trace:
        ax1[1, 1].imshow(buf4['vx'][0,0].T[::-1,:], **dx)
        ax2[1, 1].imshow(buf4['vy'][0,0].T[::-1,:], **dx)
        ax3[1, 1].imshow(buf4['vz'][0,0].T[::-1,:], **dx)
        ax1[1, 1].set_title('loadmat on .mat')

if trace:
    for ind,fig in enumerate((f1,f2,f3)):
        fig.tight_layout()
        plt.savefig(__file__.replace('.py', '%d.pdf' % ind))
    plt.show()
