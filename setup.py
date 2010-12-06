#!/Library/Frameworks/Python.framework/Versions/2.7/bin/python2.7
#
# Copyright 2010 Google Inc. All Rights Reserved.

import os
from distutils.core import setup, Extension


if not os.environ['CONFIGURATION']:
  config = 'Release'
else:
  config = os.environ['CONFIGURATION']

setup(name='pyadem',
      version='0.5',
      ext_modules=[Extension(
          name='pyadem',
          sources=['pyadem.cc'],
          include_dirs=['..', '.', '/usr/include/libxml'],
          libraries=['xml2'],
          extra_objects=['build/%s/libDiadem.a' % config],
          )],
      )
