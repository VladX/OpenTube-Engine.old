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

bool frag_pool_save (frag_pool_t * p, const char * filename)
{
	uint i, len = strlen(filename);
	FILE * f;
	char * path = (char *) malloc(config.temp_dir.len + len + 1);
	
	memcpy(path, config.temp_dir.str, config.temp_dir.len);
	memcpy(path + config.temp_dir.len, filename, len + 1);
	
	f = fopen(path, "wb");
	debug_print_3("save pool to file \"%s\"", path);
	free(path);
	
	if (f == NULL)
		return false;
	
	for (i = 0; i < p->real_len; i++)
		if (!(p->e[i].free))
			(void) fwrite(p->e[i].data, p->node_size, 1, f);
	
	fclose(f);
	
	return true;
}

frag_pool_t * frag_pool_load (uint size, uint res_len, const char * filename)
{
	FILE * f;
	frag_pool_t * p;
	long fsize;
	uint i, len = strlen(filename);
	char * path = (char *) malloc(config.temp_dir.len + len + 1);
	
	memcpy(path, config.temp_dir.str, config.temp_dir.len);
	memcpy(path + config.temp_dir.len, filename, len + 1);
	
	f = fopen(path, "rb");
	
	if (f == NULL)
		goto free_exit;
	
	if (fseek(f, 0, SEEK_END) == -1)
		goto close_free_exit;
	
	fsize = ftell(f);
	
	if (fsize == -1 || fsize % size != 0)
		goto close_free_exit;
	
	rewind(f);
	p = frag_pool_create(size, res_len);
	
	for (i = 0; i < (fsize / size); i++)
		if (fread(frag_pool_alloc(p), size, 1, f) < 1)
			goto close_free_exit;
	
	fclose(f);
	remove(path);
	debug_print_3("pool loaded from file \"%s\"", path);
	free(path);
	
	return p;
	
	close_free_exit:
	fclose(f);
	remove(path);
	free_exit:
	free(path);
	
	return NULL;
}