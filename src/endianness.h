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

#include "common_functions.h"

#ifdef _BSD
 #include <sys/endian.h>
#else
 #include <byteswap.h>
 #include <endian.h>
#endif

#ifndef bswap_32
 #ifdef bswap32
  #define bswap_32(X) bswap32(X)
 #endif
#endif

#ifndef __BYTE_ORDER
 #ifdef _BYTE_ORDER
  #define __BYTE_ORDER _BYTE_ORDER
 #endif
#endif

#ifndef __BIG_ENDIAN
 #ifdef _BIG_ENDIAN
  #define __BIG_ENDIAN _BIG_ENDIAN
 #endif
#endif

#ifndef __LITTLE_ENDIAN
 #ifdef _LITTLE_ENDIAN
  #define __LITTLE_ENDIAN _LITTLE_ENDIAN
 #endif
#endif
