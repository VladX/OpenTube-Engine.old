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

PRE_COMMIT_DIR = './git-hooks/pre-commit'

for p in os.listdir(PRE_COMMIT_DIR):
	f = os.path.join(PRE_COMMIT_DIR, p)
	if not os.path.isfile(f):
		continue
	if not os.access(f, os.X_OK):
		continue
	if os.system(f) != 0:
		sys.exit(1)
