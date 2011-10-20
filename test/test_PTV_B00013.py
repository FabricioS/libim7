#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2010 Fabricio Silva

"""
"""

import sys
sys.path.append('../libim7')
import numpy as np
trace = True
flags = [True, True, True, True]

if trace:
    import matplotlib.pyplot as plt
    # Figure1 : vx, 2:vy, 3:vz
    f1 = plt.figure(1)
    ax = f1.add_subplot(111)
    
nx, ny = 118, 78
d = {'interpolation':'nearest', 'vmin':-10, 'vmax':10, 'origin':'upper'}
# libim7
if flags[0]:
    import libim7 as im7
    buf1, att1 = im7.readim7('PTV_B00013.VC7')
    dx = {}#'extent':(buf1.x[0],buf1.x[-1],buf1.y[0],buf1.y[-1])}
    dx.update(d)
    if trace:
        ax.quiver(buf1.x, buf1.y, buf1.vx, buf1.vy, np.sqrt(buf1.vx**2+buf1.vy**2), 
                  scale=1.5)
if trace:
    plt.show()
