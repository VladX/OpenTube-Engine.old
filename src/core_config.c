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

#define CONFIG_IS_FLAG_ENABLED(F) (strcmp(F, "on") == 0 || strcmp(F, "enable") == 0 || strcmp(F, "1") == 0)


config_t config;
int http_port;
str_t http_server_tcp_addr, http_server_unix_addr;

typedef struct
{
	char * key;
	char * value;
	int line;
} conf_elem;

static const char * required_directives[] = {"listen", "user", "group", "http-temp-dir", "http-document-root"};


static void default_config (void)
{
	config.gzip = false;
	config.gzip_level = 6;
	config.gzip_min_page_size = 64;
	config.limit_req = false;
	config.limit_rate = 64;
	config.limit_delay = 30;
}

static void process_directive (conf_elem * el)
{
	if (strcmp(el->key, "listen") == 0)
	{
		char * addr, * port;
		
		for (addr = el->value, port = el->value + strlen(el->value) - 1; port >= addr; port--)
			if (* port == ':')
			{
				* port = '\0';
				port++;
				break;
			}
		
		http_port = 0;
		
		if (port != addr)
		{
			if (!(* addr))
				eerr(3, "%s", "Address is empty!"); /* ugly, but eerr() requires an at least one parameter to be specified after format string */
			http_port = atoi(port);
			if (http_port <= 0 || http_port > 65535)
				eerr(2, "Invalid port: %s", port);
			set_cpy_str(&http_server_tcp_addr, addr);
			set_cpy_str(&http_server_unix_addr, "");
		}
		else
		{
			set_cpy_str(&http_server_unix_addr, addr);
			set_cpy_str(&http_server_tcp_addr, "");
		}
		
		debug_print_1("address \"%s\", port %d", addr, http_port);
	}
	else if (strcmp(el->key, "user") == 0)
		config.user = el->value;
	else if (strcmp(el->key, "group") == 0)
		config.group = el->value;
	else if (strcmp(el->key, "http-temp-dir") == 0)
		config.temp_dir = el->value;
	else if (strcmp(el->key, "http-document-root") == 0)
	{
		config.document_root.str = (uchar *) el->value;
		config.document_root.len = strlen(el->value);
		
		if (config.document_root.str[config.document_root.len - 1] == '/')
		{
			config.document_root.len--;
			config.document_root.str[config.document_root.len] = '\0';
		}
	}
	else if (strcmp(el->key, "gzip") == 0)
	{
		if (CONFIG_IS_FLAG_ENABLED(el->value))
			config.gzip = true;
		else
			config.gzip = false;
	}
	else if (strcmp(el->key, "gzip-compression-level") == 0)
	{
		uchar gzip_compress_level;
		
		gzip_compress_level = (uchar) atoi(el->value);
		if (gzip_compress_level < 1 || gzip_compress_level > 9)
			eerr(1, "%s", "The gzip compression level must be between 1 and 9.");
		config.gzip_level = gzip_compress_level;
	}
	else if (strcmp(el->key, "gzip-min-page-size") == 0)
	{
		int gzip_page_size;
		
		gzip_page_size = atoi(el->value);
		if (gzip_page_size < 1)
			eerr(1, "Invalid value for \"%s\".", el->key);
		config.gzip_min_page_size = gzip_page_size;
	}
	else if (strcmp(el->key, "limit-requests") == 0)
	{
		if (CONFIG_IS_FLAG_ENABLED(el->value))
			config.limit_req = true;
		else
			config.limit_req = false;
	}
	else if (strcmp(el->key, "limit-requests-rate") == 0)
	{
		int rate;
		
		rate = atoi(el->value);
		if (rate <= 0)
			eerr(1, "Invalid value for \"%s\".", el->key);
		config.limit_rate = rate;
	}
	else if (strcmp(el->key, "limit-requests-delay") == 0)
	{
		int delay;
		
		delay = atoi(el->value);
		if (delay <= 0)
			eerr(1, "Invalid value for \"%s\".", el->key);
		config.limit_delay = delay;
	}
	else
		eerr(1, "Unknown directive \"%s\" in configuration file on line %d.", el->key, el->line);
}

static conf_elem * parse_line (FILE * c, int * line_num)
{
	char line[256], * ch;
	int i;
	conf_elem * el;
	
	(* line_num)++;
	
	if (fgets(line, sizeof(line), c) == NULL)
		return NULL;
	
	for (ch = line; IS_SPACE(* ch); ch++) {}
	
	el = (conf_elem *) malloc(sizeof(conf_elem));
	el->key = NULL;
	el->value = NULL;
	
	for (i = 0; !IS_SPACE(* ch); ch++, i++)
	{
		if (* ch == '#' || * ch == '\0')
			return el;
		else if (!IS_SYM(* ch))
		{
			eerr(1, "Illegal character in configuration file on line %d.", * line_num);
		}
		else
		{
			el->key = realloc(el->key, i + 2);
			el->key[i] = * ch;
			el->key[i + 1] = '\0';
		}
	}
	
	for (; IS_SPACE(* ch); ch++) {}
	
	for (i = 0; !IS_SPACE(* ch); ch++, i++)
	{
		if (* ch == '#' || * ch == '\0')
			return el;
		else if (!IS_SYM(* ch))
		{
			eerr(1, "Illegal character in configuration file on line %d.", * line_num);
		}
		else
		{
			el->value = realloc(el->value, i + 2);
			el->value[i] = * ch;
			el->value[i + 1] = '\0';
		}
	}
	
	return el;
}

void load_config (const char * path)
{
	FILE * c = fopen(path, "r");
	int line_num = 0, i = 0, it = 0;
	conf_elem * el;
	buf_t * els;
	
	if (c == NULL)
		peerr(1, "Load configuration file \"%s\"", path);
	
	default_config();
	
	els = buf_create(sizeof(conf_elem *), 32);
	
	while ((el = parse_line(c, &line_num)) != NULL)
	{
		if (el->key == NULL || el->value == NULL || !(* (el->key)) || !(* (el->value)))
		{
			free(el->key);
			free(el->value);
			free(el);
			continue;
		}
		
		buf_expand(els, 1);
		el->line = line_num;
		((conf_elem **) els->data)[i] = el;
		
		i++;
	}
	
	for (it = 0; it < ARRAY_LENGTH(required_directives); it++)
	{
		for (i = 0; i < els->cur_len; i++)
		{
			el = ((conf_elem **) els->data)[i];
			if (strcmp(el->key, required_directives[it]) == 0)
				break;
		}
		if (i == els->cur_len)
			eerr(1, "Required directive \"%s\" not found in configuration file.", required_directives[it]);
	}
	
	for (i = 0; i < els->cur_len; i++)
	{
		el = ((conf_elem **) els->data)[i];
		
		process_directive(el);
	}
	
	buf_destroy(els);
	
	fclose(c);
}
