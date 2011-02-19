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

CHECK_FILE_EXT = ['c', 'h', 'py', 'S', 'cpp']
DIR = 'src'

def check_gpl_header(path):
	
	files = []
	for p in os.listdir(path):
		f = os.path.join(path, p)
		if os.path.isdir(f):
			check_gpl_header(f)
			continue
		ext = p.split('.')[-1]
		if ext in CHECK_FILE_EXT:
			if open(f, 'r').read(2048).find('GNU General Public License') == -1:
				files += [f]
	
	return files

files_without_header = check_gpl_header(DIR)

if len(files_without_header) > 0:
	print('GNU GPL header should be included in this files:')
	for f in files_without_header:
		print(f)
	os.sys.exit(1)
