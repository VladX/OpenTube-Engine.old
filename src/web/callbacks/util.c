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

#define APPEND(PTR, LEN) append_to_buffer(input_buffer, (void *) PTR, LEN)
#define PRINT(PTR) print_to_buffer(input_buffer, (void *) PTR)

inline void append_to_buffer (buf_t * buf, void * ptr, uint len)
{
	buf_expand(buf, len);
	memcpy((uchar *) buf->data + buf->cur_len - len, ptr, len);
}

inline void print_to_buffer (buf_t * buf, void * ptr)
{
	append_to_buffer(buf, ptr, strlen((char *) ptr));
}
