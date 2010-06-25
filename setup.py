#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2006 Fabricio Silva

"""
"""
import os
from distutils.core import setup, Extension

name = 'im7'
version = 0.1

setup(name=name, version=version, \
    ext_modules=[Extension('_'+name, 
        sources=['src/ReadIM7.cpp', 'src/ReadIMX.cpp'],
        libraries=['z',],\
        define_macros=[('_LINUX', None), ], \
        extra_compile_args=['-ansi', '-pedantic', '-g']
        )])
