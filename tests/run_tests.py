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

import os
import sys
from tests import *

tests_to_run = sys.argv
if len(tests_to_run) > 0:
	tests_to_run.pop(0)

def run_tests():
	
	passed = 0
	failed = 0
	
	for p in os.listdir('./'):
		if not os.path.isfile(p):
			continue
		name = p.rstrip('py').rstrip('.')
		if len(tests_to_run) > 0 and p not in tests_to_run and name not in tests_to_run:
			continue
		try:
			m = __import__(name)
		except ImportError:
			continue
		try:
			m.run_test()
		except AttributeError:
			continue
		except UnitTestError as e:
			failed += 1
			if e.msg:
				print('Test "%s" failed: %s.' % (p, e.msg))
			else:
				print('Test "%s" failed.' % p)
			continue
		passed += 1
		print('Test "%s" passed.' % p)
	
	print('\nNumber of passed tests: %d\nNumber of failed tests: %d' % (passed, failed))
	return failed

if __name__ == '__main__':
	if run_tests() > 0:
		sys.exit(1)
