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

#include "config_parser.h"
#include <limits.h>

#define CONFIG_VALID_SYMBOL(X) (IS_SYM(X) || IS_SPACE(X) || (X) == '\r' || (X) == '\n')
#define CONFIG_VALID_NAME_SYMBOL(X) (((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z') || ((X) >= '0' && (X) <= '9') || (X) == '-' || (X) == '_')


void config_init (config_t * c)
{
	memset(c, 0, sizeof(config_t));
}

static void config_set_error_text (config_setting_t * s, const char * text)
{
	s->config->error_text = text;
	s->config->error_file = s->file;
	s->config->error_line = s->line;
}

static int config_get_line_by_ptr (config_setting_t * s, const char * ptr)
{
	const char * p = s->file_content;
	int line = 1;
	
	for (; p != ptr && !(p[0] == '\0' && p[1] == '\0'); p++)
		if (* p == '\n')
			line++;
	
	return line;
}

static void config_set_error_line (config_setting_t * s, const char * ptr)
{
	int line = config_get_line_by_ptr(s, ptr);
	
	s->line = line;
	s->config->error_line = line;
}

static bool config_quick_validate (config_setting_t * s, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++)
		if (!CONFIG_VALID_SYMBOL(s->content[i]))
		{
			config_set_error_text(s, "this is not a text file");
			config_set_error_line(s, &(s->content[i]));
			
			return false;
		}
	
	return true;
}

static bool config_load_file_contents (config_setting_t * s, FILE * f)
{
	char * content = NULL;
	char buf[READ_SIZE];
	size_t ret, cur_size = 0;
	
	while ((ret = fread(buf, 1, sizeof(buf), f)))
	{
		content = realloc(content, cur_size + ret + 2);
		if (content == NULL)
			peerr(-1, "%s", "memory error");
		memcpy(content + cur_size, buf, ret);
		cur_size += ret;
	}
	
	if (ferror(f))
	{
		if (content)
			free(content);
		
		config_set_error_text(s, "I/O error");
		
		return false;
	}
	
	if (!content)
		content = malloc(sizeof(* content));
	
	content[cur_size] = '\0';
	content[cur_size + 1] = '\0';
	s->file_content = (s->content = content);
	
	if (!config_quick_validate(s, cur_size))
	{
		if (content)
			free(content);
		
		return false;
	}
	
	return true;
}

static config_setting_t * config_find_child_by_name (config_setting_t * s, str_t * str)
{
	size_t i;
	
	for (i = 0; i < s->childs.num; i++)
		if (str->len == s->childs.elems[i].name.len && strncmp(str->str, s->childs.elems[i].name.str, str->len) == 0)
			return &(s->childs.elems[i]);
	
	return NULL;
}

static void config_jump_to_nonspace (char ** ptr)
{
	char * c = * ptr;
	
	for (; * c != '\0'; c++)
	{
		if (c[0] == '#' || (c[0] == '/' && c[1] == '/'))
		{
			c = strchr(c, '\n');
			if (c == NULL)
				break;
			continue;
		}
		
		if (c[0] == '/' && c[1] == '*')
		{
			c = strstr(c, "*/");
			if (c == NULL)
				break;
			c++;
			continue;
		}
		
		if (!(IS_SPACE(* c) || (* c) == '\r' || (* c) == '\n'))
			break;
	}
	
	if (c == NULL)
		c = (* ptr) + strlen(* ptr);
	
	* ptr = c;
}

static bool config_parse_setting_name (config_setting_t * s, char ** ptr)
{
	char * p = * ptr;
	str_t name;
	
	s->line = config_get_line_by_ptr(s, p);
	
	for (; ((* p) != '\0') && CONFIG_VALID_NAME_SYMBOL(* p); p++);
	
	name.str = * ptr;
	name.len = p - (* ptr);
	
	if (!name.len)
	{
		config_set_error_line(s, p);
		config_set_error_text(s, "name for the setting object not found");
		
		return false;
	}
	
	if (config_find_child_by_name(s->parent, &name))
	{
		config_set_error_line(s, p);
		config_set_error_text(s, "duplicate setting name");
		
		return false;
	}
	
	s->name = name;
	* ptr = p;
	
	return true;
}

static bool config_parse_string (config_setting_t * s)
{
	char * p = s->content;
	
	for (; * p != '\0' && * p != '"'; p++);
	
	if (* p == '\0')
	{
		config_set_error_line(s, p);
		config_set_error_text(s, "unexpected end of file, expected end of string (closing quote)");
		
		return false;
	}
	
	s->type = CONFIG_TYPE_STRING;
	s->strval.str = s->content;
	s->strval.len = p - s->content;
	s->strval.str[s->strval.len] = '\0';
	
	return true;
}

static char * config_parse_int (config_setting_t * s)
{
	char * endptr;
	int64 val;
	
	val = strtoll(s->content, &endptr, 0);
	
	if (* endptr == '.')
	{
		s->dval = strtod(s->content, &endptr);
		s->type = CONFIG_TYPE_FLOAT;
	}
	else if (val > INT_MAX)
	{
		s->llval = val;
		s->type = CONFIG_TYPE_INT64;
	}
	else
	{
		s->ival = (int) val;
		s->type = CONFIG_TYPE_INT;
	}
	
	config_jump_to_nonspace(&endptr);
	
	return endptr;
}

static char * config_parse_childs (config_setting_t * s);

static char * config_parse_command (config_setting_t * s, char * ptr)
{
	FILE * include_file;
	config_setting_t * parent = s;
	
	if (strncasecmp(ptr, "include", 7) != 0)
	{
		config_set_error_line(s, ptr);
		config_set_error_text(s, "unknown command after \"@\"");
		
		return NULL;
	}
	
	ptr += 7;
	config_jump_to_nonspace(&ptr);
	
	if (* ptr != '"')
	{
		config_set_error_line(s, ptr);
		config_set_error_text(s, "expected a quoted string after \"@include\"");
		
		return NULL;
	}
	
	ptr++;
	s->content = ptr;
	
	if (!config_parse_string(s))
		return NULL;
	
	s->type = CONFIG_TYPE_GROUP;
	s->strval.str[s->strval.len] = '\0';
	s->file = s->strval.str;
	s->file_content = "";
	ptr = s->content + s->strval.len + 1;
	
	while ((parent = parent->parent))
		if (strcmp(s->file, parent->file) == 0)
		{
			config_set_error_line(s, ptr);
			config_set_error_text(s, "inclusion of this file will lead to an infinite loop");
			
			return NULL;
		}
	
	config_jump_to_nonspace(&ptr);
	
	if (* ptr == ';')
		ptr++;
	
	include_file = fopen(s->file, "r");
	
	if (include_file == NULL)
	{
		config_set_error_line(s, ptr);
		config_set_error_text(s, strerror(io_errno));
		
		return NULL;
	}
	
	if (!config_load_file_contents(s, include_file))
		return NULL;
	
	fclose(include_file);
	
	if (!config_parse_childs(s))
		return NULL;
	
	return ptr;
}

static char * config_parse_childs (config_setting_t * s)
{
	char * ptr = s->content;
	config_setting_t * child;
	size_t i, k;
	
	while (* ptr != '\0')
	{
		s->type = CONFIG_TYPE_GROUP;
		
		config_jump_to_nonspace(&ptr);
		
		if (* ptr == '\0')
			break;
		
		if (* ptr == '}' && s != s->config->root)
		{
			ptr++;
			
			if (* ptr == ';')
				ptr++;
			
			break;
		}
		
		s->childs.num++;
		s->childs.elems = realloc(s->childs.elems, s->childs.num * sizeof(config_setting_t));
		
		for (i = 0; i < (s->childs.num - 1); i++)
			for (k = 0; k < s->childs.elems[i].childs.num; k++)
				s->childs.elems[i].childs.elems[k].parent = &(s->childs.elems[i]);
		
		child = &(s->childs.elems[s->childs.num - 1]);
		memset(child, 0, sizeof(config_setting_t));
		child->file_content = s->file_content;
		child->file = s->file;
		child->config = s->config;
		child->parent = s;
		
		if (* ptr == '@')
		{
			ptr++;
			
			ptr = config_parse_command(child, ptr);
			
			if (!ptr)
				break;
			
			continue;
		}
		
		if (!config_parse_setting_name(child, &ptr))
			return NULL;
		
		config_jump_to_nonspace(&ptr);
		
		if (* ptr != ':' && * ptr != '=')
		{
			config_set_error_line(child, ptr);
			config_set_error_text(child, "invalid character after the setting name (expected one of \":\" or \"=\")");
			
			return NULL;
		}
		
		child->name.str[child->name.len] = '\0';
		ptr++;
		config_jump_to_nonspace(&ptr);
		
		if (* ptr == '{')
		{
			ptr++;
			child->content = ptr;
			ptr = config_parse_childs(child);
			
			if (ptr == NULL)
				break;
			
			config_jump_to_nonspace(&ptr);
			
			if (* ptr == ';')
				ptr++;
		}
		else if (* ptr == '"')
		{
			ptr++;
			child->content = ptr;
			
			if (!config_parse_string(child))
				return NULL;
			
			ptr = child->content + child->strval.len + 1;
			config_jump_to_nonspace(&ptr);
			
			if (* ptr == ';')
				ptr++;
		}
		else if (IS_DIGIT(* ptr))
		{
			child->content = ptr;
			ptr = config_parse_int(child);
			
			if (* ptr == ';')
				ptr++;
		}
		else if (strncasecmp(ptr, "false", 5) == 0)
		{
			child->content = ptr;
			child->type = CONFIG_TYPE_BOOL;
			child->bval = false;
			
			ptr += 5;
			config_jump_to_nonspace(&ptr);
			
			if (* ptr == ';')
				ptr++;
		}
		else if (strncasecmp(ptr, "true", 4) == 0)
		{
			child->content = ptr;
			child->type = CONFIG_TYPE_BOOL;
			child->bval = true;
			
			ptr += 4;
			config_jump_to_nonspace(&ptr);
			
			if (* ptr == ';')
				ptr++;
		}
		else
		{
			config_set_error_line(child, ptr);
			config_set_error_text(child, "unrecognized value type for setting");
			
			return NULL;
		}
	}
	
	return ptr;
}

bool config_read (config_t * c, const char * filename, FILE * f)
{
	c->root = calloc(1, sizeof(config_setting_t));
	
	if (c->root == NULL)
		return false;
	
	c->root->config = c;
	c->root->file = filename;
	
	if (!config_load_file_contents(c->root, f))
		return false;
	
	return (config_parse_childs(c->root)) ? true : false;
}

const char * config_error_file (config_t * c)
{
	return c->error_file;
}

int config_error_line (config_t * c)
{
	return c->error_line;
}

const char * config_error_text (config_t * c)
{
	return c->error_text;
}

bool config_setting_path (config_setting_t * setting, char * out_buf, size_t buf_len)
{
	const char * name;
	char * path = NULL;
	
	memset(out_buf, 0, buf_len);
	
	do
	{
		name = config_setting_name(setting);
		setting = config_setting_parent(setting);
		
		if (name == NULL)
			continue;
		
		if (path == NULL)
			path = out_buf;
		else
			strcat(path, ".");
		
		if ((buf_len - strlen(path)) <= strlen(name))
			return false;
		
		strcat(path, name);
		(void) str_reverse((path + strlen(path)) - strlen(name));
	}
	while (setting != NULL);
	
	if (path != NULL)
		(void) str_reverse(path);
	
	return (path == NULL) ? false : true;
}

static config_setting_t * config_lookup_relative (config_setting_t * s, const char * path)
{
	str_t name;
	const char * pos = path;
	config_setting_t * include = NULL, * locals = s;
	size_t i;
	
	for (;;)
	{
		name.str = (char *) pos;
		pos = strchr(pos, '.');
		name.len = ((pos) ? (pos - name.str) : strlen(name.str));
		
		if (!name.len)
			return NULL;
		
		s = locals;
		locals = config_find_child_by_name(locals, &name);
		
		if (!locals)
		{
			for (i = 0; i < s->childs.num; i++)
				if (s->childs.elems[i].name.len == 0 && (include = config_lookup_relative(&(s->childs.elems[i]), name.str)))
					break;
			
			locals = include;
		}
		
		if (!locals || pos == NULL)
			break;
		
		pos++;
	}
	
	return locals;
}

config_setting_t * config_lookup (config_t * c, const char * path)
{
	return config_lookup_relative(c->root, path);
}

const char * config_setting_get_string (config_setting_t * s)
{
	if (s->type != CONFIG_TYPE_STRING)
		return NULL;
	
	return s->strval.str;
}

int config_setting_get_int (config_setting_t * s)
{
	if (s->type != CONFIG_TYPE_INT)
		return 0;
	
	return s->ival;
}

int64 config_setting_get_int64 (config_setting_t * s)
{
	if (s->type != CONFIG_TYPE_INT64)
		return 0;
	
	return s->llval;
}

double config_setting_get_float (config_setting_t * s)
{
	if (s->type != CONFIG_TYPE_FLOAT)
		return 0.0;
	
	return s->dval;
}

bool config_setting_get_bool (config_setting_t * s)
{
	return s->bval;
}

config_setting_t * config_setting_get_elem (config_setting_t * s, int i)
{
	if (i < 0 || i >= s->childs.num)
		return NULL;
	
	return &(s->childs.elems[i]);
}

const char * config_setting_name (config_setting_t * s)
{
	return s->name.str;
}

config_setting_t * config_setting_parent (config_setting_t * s)
{
	return s->parent;
}

enum CONFIG_TYPE config_setting_type (config_setting_t * s)
{
	return s->type;
}

int config_setting_source_line (config_setting_t * s)
{
	return s->line;
}

config_setting_t * config_root_setting (config_t * c)
{
	return c->root;
}

static void config_destroy_elem (config_setting_t * s)
{
	size_t i;
	
	if (s)
	{
		for (i = 0; i < s->childs.num; i++)
			config_destroy_elem(&(s->childs.elems[i]));
		
		free(s->childs.elems);
		s->childs.elems = NULL;
		s->childs.num = 0;
		
		if (s->name.len == 0 && s->type == CONFIG_TYPE_GROUP && s->file_content)
			free(s->file_content);
		
		s->file_content = NULL;
	}
}

void config_destroy (config_t * c)
{
	config_destroy_elem(c->root);
	free(c->root);
	c->root = NULL;
}
