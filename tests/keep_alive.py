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

import time
from tests import *

request_headers = [
	'GET / HTTP/1.1',
	'Connection: Keep-Alive'
]

def run_test():
	test = Tests()
	test.do(request_headers)
	time.sleep(0.2)
	response_line, headers, body = test.do(request_headers)
	
	for (k, v) in headers:
		if k.lower() == 'connection' and v.lower().find('keep-alive') == -1:
			raise UnitTestError()
	
	test = range(1000)
	for i in xrange(1000):
		test[i] = Tests()
		response_line, headers, body = test[i].do(request_headers)
		for (k, v) in headers:
			if k.lower() == 'connection' and v.lower().find('keep-alive') == -1:
				print('Keep-Alive max connections: %d' % i)
				return
