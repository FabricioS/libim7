#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2006 Fabricio Silva

"""
"""
import sys,os
from numpy.distutils.core import setup, Extension
from numpy.distutils.command.build_ext import build_ext

name = 'im7'
version = 0.1

# Configure C and fortran compilers
if sys.platform in ('win32', 'cygwin'):
    ext = []
else:
    ext = [Extension('_'+name, 
        sources=['src/ReadIM7.cpp', 'src/ReadIMX.cpp'],
        libraries=['z',],\
        define_macros=[('_LINUX', None), ], \
        extra_compile_args=['-ansi', '-pedantic', '-g']),]
    
setup(name=name, version=version, \
    ext_modules=ext)
