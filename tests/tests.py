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

import sys
import socket
import re

class Tests:
	
	sock = None
	host = 'localhost'
	port = 80
	
	def __init__(self, h = None, p = None):
		
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		if h is not None:
			self.host = h
		if p is not None:
			self.port = p
		try:
			self.sock.connect((self.host, self.port))
		except socket.error as e:
			print('Connection error: %s.' % e.strerror)
			sys.exit(1)
	
	def __del__(self):
		
		self.sock.close()
	
	def send(self, buf):
		
		try: # Python 3.x
			self.sock.send(bytes(buf, 'ascii'))
		except TypeError:
			self.sock.send(buf)
	
	def recv(self, count):
		
		r = self.sock.recv(count)
		try:
			r = r.decode('ascii')
		except: pass
		return r
	
	def parse(self, buf):
		
		response_line = {'ver': '', 'code': 0, 'code_msg': ''}
		headers = []
		body = []
		buf = buf.split('\r\n')
		s = buf.pop(0)
		
		m = re.match('([\w\./0-9]+) (\d{3}) ([\w ]+)', s)
		if m is not None:
			response_line['ver'] = m.group(1)
			response_line['code'] = int(m.group(2))
			response_line['code_msg'] = m.group(3)
		
		for i in range(len(buf)):
			s = buf.pop(0)
			if s == '':
				break
			m = re.match('([^ :]+): ?(.*)', s)
			if m is not None:
				headers.append((m.group(1), m.group(2)))
		
		body = ''.join(buf)
		return (response_line, headers, body)
	
	def do(self, headers, body = '', recv_buf = 65536):
		
		r = []
		r += list(headers)
		r.append('')
		r.append(str(body))
		r = '\r\n'.join(r)
		self.send(r)
		r = self.sock.recv(recv_buf)
		try:
			r = r.decode('ascii')
		except: pass
		return self.parse(r)

class UnitTestError(Exception):
	
	def __init__(self, msg = ''):
		self.msg = msg
