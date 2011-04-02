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
import json

lang = json.load(open('strings.json', 'r'))
maxid = 0
ids = []

for s in lang:
	try:
		i = int(s)
	except:
		i = False
	if not i:
		continue
	maxid = max(i, maxid)
	ids.append(i)

ids.sort()

f = open('strings.h', 'w')
f.write('''/* Please, do not touch this file. Edit strings.json instead. */

static const char * strings[%d] = {''' % (maxid + 1))

for i in range(maxid + 1):
	if i in ids:
		val = '"%s"' % (lang[str(i)].replace('"', '\\"'))
	else:
		val = 'NULL'
	f.write(val)
	if i != maxid:
		f.write(', ')

f.write('};\n')
f.close()
