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

def run_test():
	test = Tests()
	line = 'GET '
	for i in xrange(10000):
		line += '/veeeeeeeeeeeeeeeeeeeeeeeeeeeeeryloooooooooooooooooooooooooooooooooooooooong'
	line += ' HTTP/1.0'
	response_line, headers, body = test.do([line])
	
	if response_line['code'] != 414:
		raise UnitTestError('Server return %d response code for long request line' % response_line['code'])
