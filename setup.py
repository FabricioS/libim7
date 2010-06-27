#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2006 Fabricio Silva

"""
"""
import sys,os
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

name = 'im7'
version = 0.1

# Configure C and fortran compilers
if sys.platform in ('win32', 'cygwin'):
    build_ext.compiler = "mingw32"
    build_ext.fcompiler = "gnu"
else:
    build_ext.compiler = "unix"
    build_ext.fcompiler = "gnu95"
    
setup(name=name, version=version, \
    ext_modules=[Extension('_'+name, 
        sources=['src/ReadIM7.cpp', 'src/ReadIMX.cpp'],
        libraries=['z',],\
        define_macros=[('_LINUX', None), ], \
        extra_compile_args=['-ansi', '-pedantic', '-g']
        )])
