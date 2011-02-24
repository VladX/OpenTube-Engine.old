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
init_callbacks = []
inc = script_info
clb = script_info
init_clb = script_info
regex_clb = re.compile('WEB_CALLBACK *\(\s*(\w+)\s*,\s*([^\)]+)\)\s*{')
regex_init_clb = re.compile('WEB_INIT *\(\s*(\w+)\s*\)\s*{')

def remove_duplicates(l):
	seen = {}
	result = []
	for item in l:
		if item in seen:
			continue
		seen[item] = 1
		result.append(item)
	return result

try:
	DIR = os.sys.argv[1]
except IndexError:
	print('Usage: %s DIR' % os.sys.argv[0])
	os.sys.exit(1)

for f in glob.glob(os.path.join(DIR, '*.c')):
	cont = open(f, 'r').read()
	
	m = regex_clb.search(cont)
	if m is not None:
		files.append(os.path.basename(f))
		for m in regex_clb.finditer(cont):
			callbacks.append((m.group(1), m.group(2)))
	
	m = regex_init_clb.search(cont)
	if m is not None:
		files.append(os.path.basename(f))
		for m in regex_init_clb.finditer(cont):
			init_callbacks.append(m.group(1))

files = remove_duplicates(files)
init_callbacks = remove_duplicates(init_callbacks)

tpl = open(os.path.join(DIR, 'callbacks/callbacks.c.tpl'), 'r').read()

for f in files:
	inc += '\n#include "../%s"' % f

for c in callbacks:
	clb += '\n\tweb_set_callback(WEB_CALLBACK_TO_C_FUNC(%s), %s);' % c

for c in init_callbacks:
	init_clb += '\n\tWEB_INIT_CALLBACK_TO_C_FUNC(%s) ();' % c

cont = tpl.replace('/*--INCLUSION--*/', inc).replace('/*--CALLBACKS-SETUP--*/', clb).replace('/*--INITIALIZATION--*/', init_clb)

f = open(os.path.join(DIR, 'callbacks/callbacks.c'), 'w')
f.write(cont)
f.close()
