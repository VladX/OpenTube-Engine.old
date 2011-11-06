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

#include "common_functions.h"
#include "static_content_preprocessor.h"
#include "memcache.h"
#include <pthread.h>

extern char * cur_template_dir;

static u_str_t * static_content_update_callback (time_t time, u_str_t * name)
{
	/* TODO */
	
	return NULL;
}

static void _strip_spaces (str_big_t * content)
{
	uint64 i, k;
	
	for (i = 0, k = 0; i < content->len; i++)
	{
		if (IS_SPACE(content->str[i]))
			continue;
		
		if (i != k)
			content->str[k] = content->str[i];
		
		k++;
	}
	
	content->len = k;
}

u_str_t static_content_preprocess_and_cache (const char * name, const char ** path_list, const uint path_list_len, const bool strip_spaces)
{
	static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};
	umlong cwd;
	int i;
	u_str_t ret, nme;
	str_big_t * content = alloca(sizeof(str_big_t) * path_list_len);
	
	assert(path_list_len > 0);
	assert(path_list != NULL);
	memset(&ret, 0, sizeof(ret));
	
	pthread_mutex_lock(mutex);
	
	cwd = save_and_change_cwd(cur_template_dir);
	
	if (cwd == -1)
	{
		perr("chdir(%s) failed", cur_template_dir);
		pthread_mutex_unlock(mutex);
		
		return ret;
	}
	
	for (i = 0; i < path_list_len; i++)
	{
		if (!load_file_contents(path_list[i], content + i))
		{
			perr("can't load \"%s\"", path_list[i]);
			restore_cwd(cwd);
			pthread_mutex_unlock(mutex);
			
			for (i--; i >= 0; i--)
				allocator_free(content[i].str);
			
			return ret;
		}
		
		if (strip_spaces)
			_strip_spaces(content + i);
	}
	
	for (i = 1; i < path_list_len; i++)
	{
		content[0].len += content[i].len;
		content[0].str = allocator_realloc(content[0].str, content[0].len);
		memcpy(content[0].str + (content[0].len - content[i].len), content[i].str, content[i].len);
		allocator_free(content[i].str);
		content[i].str = NULL;
	}
	
	restore_cwd(cwd);
	
	set_ustr(&nme, (const uchar *) name);
	ret.len = content[0].len;
	ret.str = (uchar *) content[0].str;
	cache_store(&nme, &ret, static_content_update_callback);
	allocator_free(content[0].str);
	
	ret.len = config.cache_prefix.len + nme.len;
	ret.str = allocator_malloc(ret.len + 1);
	strcpy((char *) ret.str, (const char *) config.cache_prefix.str);
	strcat((char *) ret.str, (const char *) nme.str);
	
	pthread_mutex_unlock(mutex);
	
	return ret;
}
