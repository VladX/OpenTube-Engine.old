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
import hashlib
import re

index_files = ['win32/index.md5', 'win64/index.md5']

def check_index(indexfile):
	
	f = open(indexfile, 'r')
	while True:
		line = f.readline()
		if not line:
			break
		m = re.match('^([0-9a-f]{32})\s+(.+)$', line)
		if not m:
			return False
		if m.group(1) != hashlib.md5(open(os.path.dirname(indexfile) + os.path.sep + m.group(2), 'r').read()).hexdigest():
			return False
	return True

for f in index_files:
	if not check_index(f):
		print('Invalid index file "%s"' % f)
		os.sys.exit(1)
