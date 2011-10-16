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

threadsafe template_context_t tpl_ctx;

WEB_CALLBACK(video, "/v/", false)
{
	template_t tpl;
	
	tpl = tpl_compile("video.tpl");
	
	if (tpl == NULL)
		internal_server_error();
	else
	{
		tpl_set_var_static(tpl_ctx, "title", _l(1));
		tpl_complete(tpl, tpl_ctx, NULL, 0, thread_global_buffer);
	}
	
	return thread_global_buffer;
}

WEB_INIT(video)
{
	tpl_ctx = tpl_context_create();
}
