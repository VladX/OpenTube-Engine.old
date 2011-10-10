#!/usr/bin/env python
#-*- coding:utf-8 -*-
# 
# This file is part of Opentube - Open video hosting engine
# 
# Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
# 
# Opentube is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# Opentube is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Opentube.  If not, see <http://www.gnu.org/licenses/>.

import os

CHECK_FILE_EXT = ['c', 'h', 'cpp']
DIR = 'src'

def check_newline_eof(path):
	
	files = []
	for p in os.listdir(path):
		f = os.path.join(path, p)
		if os.path.isdir(f):
			check_newline_eof(f)
			continue
		ext = p.split('.')[-1]
		if ext in CHECK_FILE_EXT:
			fo = open(f, 'r')
			try:
				fo.seek(-1, 2)
			except:
				continue
			if fo.read(1) != '\n':
				files += [f]
	
	return files

files_without_newline_at_eof = check_newline_eof(DIR)

if len(files_without_newline_at_eof) > 0:
	print('Please add newline at the end of this files:')
	for f in files_without_newline_at_eof:
		print(f)
	os.sys.exit(1)
