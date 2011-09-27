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
#include "config_parser.h"
#include "win32_utils.h"

#define EUNKNOWNDIRECTIVE eerr(-1, "Unknown directive \"%s\" of type %s in configuration file on line %d. Make sure you use the correct type for this directive.", key, directive_type, line)
#define EINVALIDVAL eerr(-1, "Invalid value for \"%s\".", key)


struct loaded_config config;
int http_port;
str_t http_server_tcp_addr, http_server_unix_addr;
const char * path_to_configuration_file = NULL;

static const char * required_directives[] = {
	"server.listen", "server.user", "server.group", "server.pid-file", "server.log-file", "server.data",
	"http.document-root", "http.temp", "http.cache.prefix", "http.cache.update",
	"template.name"
};


static void default_config (void)
{
	config.gzip = false;
	config.gzip_level = 6;
	config.gzip_min_page_size = 64;
	config.limit_req = false;
	config.limit_rate = 64;
	config.limit_delay = 30;
	config.limit_sim_req = false;
	config.limit_sim_threshold = 8;
	config.keepalive_timeout.str = (uchar *) "25";
	config.keepalive_timeout.len = 2;
	config.keepalive_max_conn_per_client = 4;
	config.cache_prefix.str = (uchar *) "/cache/";
	config.cache_prefix.len = strlen((char *) config.cache_prefix.str);
	config.cache_update = 0;
	config.tpl_cache_update = 0;
	config.worker_threads = 5;
	config.prealloc_request_structures = 10;
	config.idle_request_structures = 0;
	config.script_init = "init.js";
	config.script_update = 0;
}

static void process_directive_string (const char * const key, char * value, const int line)
{
	const char * directive_type = "string";
	
	if (strcmp(key, "server.listen") == 0)
	{
		char * addr, * port;
		
		for (addr = value, port = value + strlen(value) - 1; port >= addr; port--)
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
				eerr(-1, "%s", "Address is empty!"); /* ugly, but eerr() requires an at least one parameter to be specified after format string */
			http_port = atoi(port);
			if (!is_num(port) || http_port <= 0 || http_port > 65535)
				eerr(-1, "Invalid port: %s", port);
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
	else if (strcmp(key, "server.user") == 0)
		config.user = value;
	else if (strcmp(key, "server.group") == 0)
		config.group = value;
	else if (strcmp(key, "server.pid-file") == 0)
	{
		uint pidlen = strlen(value);
		if (pidlen < 3 || value[pidlen - 1] == '/')
			EINVALIDVAL;
		if (!is_path_absolute(value))
			eerr(-1, "You should specify absolute path name for the \"%s\"", key);
		config.pid = value;
	}
	else if (strcmp(key, "server.log-file") == 0)
	{
		uint loglen = strlen(value);
		if (loglen < 3 || value[loglen - 1] == '/')
			EINVALIDVAL;
		if (!is_path_absolute(value))
			eerr(-1, "You should specify absolute path name for the \"%s\"", key);
		config.log = value;
	}
	else if (strcmp(key, "http.document-root") == 0)
	{
		set_ustr(&(config.document_root), (uchar *) value);
		
		if (config.document_root.str[config.document_root.len - 1] == '/')
		{
			config.document_root.len--;
			config.document_root.str[config.document_root.len] = '\0';
		}
		
		if (!is_directory_exists((const char *) config.document_root.str, IOAR_R))
			eerr(-1, "Directory \"%s\" does not have read permissions or does not exist.", (const char *) config.document_root.str);
	}
	else if (strcmp(key, "http.temp") == 0)
	{
		set_ustr(&(config.temp_dir), (uchar *) value);
		
		if (!is_directory_exists((const char *) config.temp_dir.str, IOAR_RW))
			eerr(-1, "Directory \"%s\" does not have read/write permissions or does not exist.", (const char *) config.temp_dir.str);
		
		if (config.temp_dir.str[config.temp_dir.len - 1] != '/')
		{
			config.temp_dir.str[config.temp_dir.len] = '/';
			config.temp_dir.len++;
		}
		
		config.temp_dir.str = allocator_realloc(config.temp_dir.str, config.temp_dir.len + 10);
	}
	else if (strcmp(key, "server.data") == 0)
	{
		set_ustr(&(config.data), (uchar *) value);
		
		if (config.data.str[config.data.len - 1] == '/')
		{
			config.data.len--;
			config.data.str[config.data.len] = '\0';
		}
		
		if (!is_directory_exists((const char *) config.data.str, IOAR_R))
			eerr(-1, "Directory \"%s\" does not have read permissions or does not exist.", (const char *) config.data.str);
	}
	else if (strcmp(key, "template.name") == 0)
	{
		set_ustr(&(config.template_name), (uchar *) value);
		
		if (!(* config.template_name.str))
			EINVALIDVAL;
	}
	else if (strcmp(key, "template.update-bytecode") == 0)
	{
		if (strcmp(value, "never") == 0)
			config.tpl_cache_update = 0;
		else if (strcmp(value, "source-modified") == 0)
			config.tpl_cache_update = 1;
		else
			EINVALIDVAL;
	}
	else if (strcmp(key, "http.cache.prefix") == 0)
	{
		int len = strlen(value);
		
		if (len < 2 || value[0] != '/')
			EINVALIDVAL;
		if (value[len - 1] == '/' && len < 3)
			EINVALIDVAL;
		if (value[len - 1] != '/')
		{
			len++;
			value = allocator_realloc(value, len + 1);
			value[len - 1] = '/';
			value[len] = '\0';
		}
		config.cache_prefix.str = (uchar *) value;
		config.cache_prefix.len = len;
	}
	else if (strcmp(key, "http.cache.update") == 0)
	{
		if (strcmp(value, "never") == 0)
			config.cache_update = 0;
		else if (strcmp(value, "source-modified") == 0)
			config.cache_update = 1;
		else
			EINVALIDVAL;
	}
	else if (strcmp(key, "script.init") == 0)
	{
		config.script_init = value;
	}
	else if (strcmp(key, "script.update-bytecode") == 0)
	{
		if (strcmp(value, "never") == 0)
			config.script_update = 0;
		else if (strcmp(value, "source-modified") == 0)
			config.script_update = 1;
		else
			EINVALIDVAL;
	}
	else if (strcmp(key, "server.idle-request-structures") == 0)
	{
		if (strcmp(value, "keep") == 0)
			config.idle_request_structures = 0;
		else if (strcmp(value, "free") == 0)
			config.idle_request_structures = 1;
		else
			EINVALIDVAL;
	}
	else
		EUNKNOWNDIRECTIVE;
}

static void process_directive_int (const char * const key, const int value, const int line)
{
	const char * directive_type = "integer";
	
	if (strcmp(key, "server.worker-threads") == 0)
	{
		if (value < 2)
			EINVALIDVAL;
		config.worker_threads = value;
	}
	else if (strcmp(key, "server.pre-allocated-request-structures") == 0)
	{
		if (value < 2)
			EINVALIDVAL;
		config.prealloc_request_structures = value;
	}
	else if (strcmp(key, "server.limits.requests.rate") == 0)
	{
		if (value <= 0)
			EINVALIDVAL;
		config.limit_rate = value;
	}
	else if (strcmp(key, "server.limits.requests.delay") == 0)
	{
		if (value <= 0)
			EINVALIDVAL;
		config.limit_delay = value;
	}
	else if (strcmp(key, "server.limits.simultaneous-requests.threshold") == 0)
	{
		if (value <= 0)
			EINVALIDVAL;
		config.limit_sim_threshold = value;
	}
	else if (strcmp(key, "http.keepalive.timeout") == 0)
	{
		if (value <= 0)
			EINVALIDVAL;
		config.keepalive_timeout.len = digits_in_int(value);
		config.keepalive_timeout.str = (uchar *) allocator_malloc(config.keepalive_timeout.len + 1);
		config.keepalive_timeout_val = (uint) value;
		int_to_str(value, (char *) config.keepalive_timeout.str, 10);
	}
	else if (strcmp(key, "http.keepalive.max-conn-per-client") == 0)
	{
		if (value <= 0)
			EINVALIDVAL;
		config.keepalive_max_conn_per_client = (uint) value;
	}
	else if (strcmp(key, "http.gzip.compression-level") == 0)
	{
		if (value < 1 || value > 9)
			eerr(-1, "%s", "The gzip compression level must be between 1 and 9.");
		config.gzip_level = value;
	}
	else if (strcmp(key, "http.gzip.threshold") == 0)
	{
		if (value < 1)
			EINVALIDVAL;
		config.gzip_min_page_size = value;
	}
	else
		EUNKNOWNDIRECTIVE;
}

static void process_directive_int64 (const char * const key, const int64 value, const int line)
{
	const char * directive_type = "integer";
	
	EUNKNOWNDIRECTIVE;
}

static void process_directive_float (const char * const key, const float value, const int line)
{
	const char * directive_type = "float";
	
	EUNKNOWNDIRECTIVE;
}

static void process_directive_bool (const char * const key, const int value, const int line)
{
	const char * directive_type = "boolean";
	
	if (strcmp(key, "http.gzip.enable") == 0)
	{
		if (value)
			config.gzip = true;
		else
			config.gzip = false;
	}
	else if (strcmp(key, "server.limits.requests.limit") == 0)
	{
		if (value)
			config.limit_req = true;
		else
			config.limit_req = false;
	}
	else if (strcmp(key, "server.limits.simultaneous-requests.limit") == 0)
	{
		if (value)
			config.limit_sim_req = true;
		else
			config.limit_sim_req = false;
	}
	else
		EUNKNOWNDIRECTIVE;
}

static bool process_conf (config_setting_t * setting)
{
	const char * string;
	char * value;
	char name[256];
	int type, i;
	
	if (setting == NULL)
		return false;
	
	type = config_setting_type(setting);
	(void) config_setting_path(setting, name, sizeof(name));
	
	if (type == CONFIG_TYPE_STRING)
	{
		string = config_setting_get_string(setting);
		value = allocator_calloc(strlen(string) + 1, sizeof(* value));
		if (value == NULL)
			peerr(-1, "%s", "Memory error");
		strcpy(value, string);
		process_directive_string(name, value, config_setting_source_line(setting));
	}
	else if (type == CONFIG_TYPE_INT)
		process_directive_int(name, config_setting_get_int(setting), config_setting_source_line(setting));
	else if (type == CONFIG_TYPE_INT64)
		process_directive_int64(name, config_setting_get_int64(setting), config_setting_source_line(setting));
	else if (type == CONFIG_TYPE_FLOAT)
		process_directive_float(name, config_setting_get_float(setting), config_setting_source_line(setting));
	else if (type == CONFIG_TYPE_BOOL)
		process_directive_bool(name, config_setting_get_bool(setting), config_setting_source_line(setting));
	else if (type == CONFIG_TYPE_GROUP)
	{
		for (i = 0; process_conf(config_setting_get_elem(setting, i)); i++);
	}
	
	return true;
}

void load_config (const char * path)
{
	FILE * conf_file = fopen(path, "r");
	config_t conf[1];
	uint i;
	
	if (conf_file == NULL)
		peerr(-1, "Load configuration file \"%s\"", path);
	
	config_init(conf);
	
	if (!config_read(conf, path, conf_file))
		eerr(-1, "Error in configuration file \"%s\" on line %d: %s.", config_error_file(conf), config_error_line(conf), config_error_text(conf));
	
	for (i = 0; i < ARRAY_LENGTH(required_directives); i++)
	{
		if (config_lookup(conf, required_directives[i]) == NULL)
			eerr(-1, "Required directive \"%s\" not found in configuration file.", required_directives[i]);
	}
	
	path_to_configuration_file = path;
	default_config();
	(void) process_conf(config_root_setting(conf));
	
	config_destroy(conf);
	fclose(conf_file);
}
