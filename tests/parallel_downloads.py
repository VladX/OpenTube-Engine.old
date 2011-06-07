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
import time

def run_test():
	first = Tests()
	second = Tests()
	string = '0' * (1024 ** 2)
	hdr = [
		'POST /echo HTTP/1.0',
		'Content-Type: application/x-www-form-urlencoded',
		'Content-Length: %d' % (len(string))
	]
	string = '\r\n'.join(hdr + ['', '']) + 'x=' + string
	first.send(string)
	second.send(string)
	first.recv(1)
	second.sock.setblocking(0)
	time.sleep(1)
	try:
		second.recv(1)
	except socket.error:
		raise UnitTestError()