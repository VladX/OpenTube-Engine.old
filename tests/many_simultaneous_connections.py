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

from tests import *
import time
import sys
import errno

maxconn = 100

def run_test():
	counter = 0
	conn = list(range(maxconn))
	for i in range(maxconn):
		conn[i] = Tests()
		try:
			conn[i].send('POST / HTTP/1.1')
		except:
			break
		conn[i].sock.setblocking(0)
		time.sleep(0.2)
		try:
			sys.stdout.write('.')
			sys.stdout.flush()
		except: pass
		try:
			conn[i].sock.recv(0)
		except socket.error as e:
			if e.errno == 11 or e.errno == 10035:
				continue
			else:
				break
	
	try:
		sys.stdout.write('\n')
	except: pass
	
	if i == 0:
		raise UnitTestError()
	
	print('Simultaneous connections limit: %s' % (('more or equal %d' % (maxconn)) if (i == maxconn - 1) else str(i)))
