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

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H 1

#include "common_functions.h"

enum CONFIG_TYPE
{
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_BOOL,
	CONFIG_TYPE_INT,
	CONFIG_TYPE_INT64,
	CONFIG_TYPE_FLOAT,
	CONFIG_TYPE_GROUP
};

typedef struct config_setting_t
{
	str_t name;
	enum CONFIG_TYPE type;
	union
	{
		str_t strval;
		int ival;
		int64 llval;
		double dval;
		bool bval;
	};
	struct config_setting_t * parent;
	struct config_t * config;
	const char * file;
	char * content;
	char * file_content;
	int line;
	struct
	{
		struct config_setting_t * elems;
		size_t num;
	} childs;
} config_setting_t;

typedef struct config_t
{
	config_setting_t * root;
	const char * error_text;
	const char * error_file;
	int error_line;
} config_t;


void config_init (config_t * c);

void config_destroy (config_t * c);

bool config_read (config_t * c, const char * filename, FILE * f);

config_setting_t * config_lookup (config_t * c, const char * path);

const char * config_setting_get_string (config_setting_t * s);

int config_setting_get_int (config_setting_t * s);

int64 config_setting_get_int64 (config_setting_t * s);

double config_setting_get_float (config_setting_t * s);

bool config_setting_get_bool (config_setting_t * s);

config_setting_t * config_setting_get_elem (config_setting_t * s, int i);

bool config_setting_path (config_setting_t * setting, char * out_buf, size_t buf_len);

const char * config_setting_name (config_setting_t * s);

config_setting_t * config_setting_parent (config_setting_t * s);

enum CONFIG_TYPE config_setting_type (config_setting_t * s);

int config_setting_source_line (config_setting_t * s);

config_setting_t * config_root_setting (config_t * c);

const char * config_error_file (config_t * c);

int config_error_line (config_t * c);

const char * config_error_text (config_t * c);

#endif
