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

request_headers = [
	[
		'GET \0/400/ HTTP/1.1',
		'Header: value'
	],
	[
		'GET /400\0/ HTTP/1.1',
		'Header: value'
	],
	[
		'GET /400/ HTT\0P/1.1',
		'Header: value'
	],
	[
		'GET /400/',
		'Header: value'
	],
	[
		'GET /400/'
	],
	[
		'GET /400/ HTTP/1.1',
		'Hea\0der: value'
	],
	[
		'GET '
	]
]

def run_test():
	for i in xrange(len(request_headers)):
		test = Tests()
		response_line, headers, body = test.do(request_headers[i])
		
		if response_line['code'] != 400:
			raise UnitTestError('Server return %d response code for wrong headers (%d)' % (response_line['code'], i))
