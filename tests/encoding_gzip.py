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
from io import BytesIO
from gzip import GzipFile

request_headers = [
	'POST /echo HTTP/1.1',
	'Accept-Encoding: gzip',
	'Content-Type: application/x-www-form-urlencoded'
]
	
def run_test():
	test_string_buffer = '0' * 16
	for i in range(16):
		test = Tests()
		req_body = 'x=' + test_string_buffer
		response_line, headers, body = test.do(request_headers + ['Content-Length: %d' % (len(req_body))], req_body)
		
		gzipped_output = False
		
		for (k, v) in headers:
			if k.lower() == 'content-encoding' and v.lower().find('gzip') != -1:
				gzipped_output = True
				break
		
		if gzipped_output:
			print('Gzip minimum page size: <= %d' % len(test_string_buffer))
			break
		
		test_string_buffer *= 2
	
	if not gzipped_output:
		return
	
	try:
		s = GzipFile('', 'r', 0, BytesIO(body)).read()
	except IOError:
		raise UnitTestError('Server return broken gzipped data')
	if str(s.decode('ascii')) != test_string_buffer:
		raise UnitTestError('Strings are not equal')