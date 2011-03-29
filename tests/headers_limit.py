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

from tests import *

max_headers = 10000

def run_test():
	test = Tests()
	test.sock.send('GET / HTTP/1.0\r\n')
	hdr = 'Header: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n'
	limit = 0
	for i in xrange(max_headers):
		try:
			test.sock.send(hdr)
			limit += len(hdr)
		except socket.error:
			break
	
	if i == max_headers - 1:
		test.sock.send('\r\n')
	else:
		print('Headers size limit: about %d bytes' % (limit))
		code = test.sock.recv(20).split(' ')[1]
		code = int(code)
		if code != 414:
			raise UnitTestError('Server return %d response code for long request line' % response_line['code'])
