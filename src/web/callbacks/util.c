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

#define APPEND(PTR, LEN) append_to_buffer((void *) PTR, LEN)
#define PRINT(PTR) print_to_buffer((void *) PTR)

inline void append_to_buffer (const void * ptr, uint len)
{
	buf_expand(thread_global_buffer, len);
	memcpy((uchar *) thread_global_buffer->data + thread_global_buffer->cur_len - len, ptr, len);
}

inline void print_to_buffer (const void * ptr)
{
	append_to_buffer(ptr, strlen((const char *) ptr));
}

inline void internal_server_error (void)
{
	web_raise(500);
}

inline void set_content_type (uchar * str, uint len)
{
	thread_request->out.content_type.str = str;
	thread_request->out.content_type.len = len;
}

inline void disable_page_compression (void)
{
	extern threadsafe volatile bool thread_allow_compression;
	thread_allow_compression = false;
}
