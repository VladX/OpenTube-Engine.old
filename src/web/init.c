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

#include <pthread.h>

WEB_INIT(init)
{
	
}

WEB_INIT(init_once)
{
	static bool once = false;
	static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};
	u_str_t uri;
	const char * path_list[] = {"css/normalize.css", "css/common.css"};
	
	pthread_mutex_lock(mutex);
	
	if (once)
	{
		pthread_mutex_unlock(mutex);
		
		return;
	}
	
	once = true;
	pthread_mutex_unlock(mutex);
	
	uri = static_content_preprocess_and_cache("style.css", path_list, ARRAY_LENGTH(path_list), false);
	tpl_set_global_var_static("template-stylesheet", (const char *) uri.str);
	tpl_set_global_var_static("lang", localization_get_lang());
}
