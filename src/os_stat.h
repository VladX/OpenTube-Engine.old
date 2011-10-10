/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>

#ifndef S_IFMT
 #ifdef _S_IFMT
  #define S_IFMT _S_IFMT
 #endif
#endif

#ifndef S_IFREG
 #ifdef _S_IFREG
  #define S_IFREG _S_IFREG
 #endif
#endif

#ifndef S_IFDIR
 #ifdef _S_IFDIR
  #define S_IFDIR _S_IFDIR
 #endif
#endif

#ifndef __S_ISTYPE
 #define __S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))
#endif

#ifndef S_ISREG
 #define S_ISREG(mode) __S_ISTYPE((mode), S_IFREG)
#endif

#ifndef S_ISDIR
 #define S_ISDIR(mode) __S_ISTYPE((mode), S_IFDIR)
#endif

#ifndef S_IXUSR
 #ifdef _S_IEXEC
  #define S_IXUSR _S_IEXEC
 #endif
#endif

#ifndef S_IWUSR
 #ifdef _S_IWRITE
  #define S_IWUSR _S_IWRITE
 #endif
#endif

#ifndef S_IRUSR
 #ifdef _S_IREAD
  #define S_IRUSR _S_IREAD
 #endif
#endif
