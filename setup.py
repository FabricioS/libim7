#!/usr/bin/python
# -*- coding: utf-8 -*-

#Copyright (C) 2006 Fabricio Silva

"""
"""
import sys,os,info
from numpy.distutils.misc_util import Configuration
from numpy.distutils.core import setup, Extension
from numpy.distutils.command.build_ext import build_ext

# Configure C and fortran compilers
    
def configure(parent_package='', top_path=None):
    config = Configuration('lib'+info.name, parent_package,top_path)
    config.add_data_dir('test')
    if sys.platform not in ('win32', 'cygwin'):
      config.add_extension('_'+info.name,
        sources=['src/ReadIM7.cpp', 'src/ReadIMX.cpp'],
        libraries=['z',],
        define_macros=[('_LINUX', None), ],
        extra_compile_args=['-ansi', '-pedantic', '-g'])
    return config
    
    
setup(configuration=configure,
  author=info.author, \
  author_email=info.author_email, \
  maintainer=info.maintainer, \
  maintainer_email=info.maintainer_email, \
  url=info.url, \
  version=info.version, \
  license=info.license, \
  long_description=info.description, \
  description=info.summary, \
  platforms=info.platforms, \
  requires=info.requires, \
  provides=[info.name], \
  py_modules=["libim7.py"])
