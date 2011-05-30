/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - VladX; http://vladx.net/
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
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
