#!/usr/bin/env python
#-*- coding:utf-8 -*-
# 
# This file is part of Opentube - Open video hosting engine
# 
# Copyright (C) 2011 - VladX; http://vladx.net/
# 
# Opentube is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# Opentube is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Opentube; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, 
# Boston, MA  02110-1301  USA

import os
import re
import glob

script_info = '/* Automatically inserted by script generate-callbacks.c.py */'
files = []
callbacks = []
inc = script_info
clb = script_info
regex = re.compile('WEB_FUNCTION *\((\w+)\s*,\s*([^\)]+)\)\s*{')

try:
	DIR = os.sys.argv[1]
except IndexError:
	print('Usage: %s DIR' % os.sys.argv[0])
	os.sys.exit(1)

for f in glob.glob(os.path.join(DIR, '*.c')):
	cont = open(f, 'r').read()
	m = regex.search(cont)
	if m is not None:
		files.append(os.path.basename(f))
		for m in regex.finditer(cont):
			callbacks.append((m.group(1), m.group(2)))

tpl = open(os.path.join(DIR, 'callbacks/callbacks.c.tpl'), 'r').read()

for f in files:
	inc += '\n#include "../%s"' % f

for c in callbacks:
	clb += '\n\tweb_set_callback(%s, %s);' % c

cont = tpl
cont = cont.replace('/*--INCLUSION--*/', inc)
cont = cont.replace('/*--CALLBACKS-SETUP--*/', clb)

f = open(os.path.join(DIR, 'callbacks/callbacks.c'), 'w')
f.write(cont)
f.close()
