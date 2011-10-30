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
#include "libs/mongo.h"

struct mongo_server
{
	const char * host;
	uint port;
};

mongo mongo_connection[1];

static const char * errorcodetomsg (int err)
{
	switch (err)
	{
		case MONGO_CONN_ADDR_FAIL:
			return "an error occured while trying to resolve address";
			break;
		case MONGO_CONN_NO_SOCKET:
			return "could not create a socket";
			break;
		case MONGO_CONN_FAIL:
			return "connection failed";
			break;
		case MONGO_CONN_NOT_MASTER:
			return "connected to a non-master node";
			break;
		case MONGO_CONN_BAD_SET_NAME:
			return "given rs name doesn't match this replica set";
			break;
		case MONGO_CONN_NO_PRIMARY:
			return "can't find primary in replica set";
			break;
		
		case MONGO_IO_ERROR:
			return "an error occurred while reading or writing on the socket";
			break;
		case MONGO_READ_SIZE_ERROR:
			return "the response is not the expected length";
			break;
		case MONGO_COMMAND_FAILED:
			return "the command returned with 'ok' value of 0";
			break;
	}
	
	return NULL;
}

static const char * parse_server (const char * servers, struct mongo_server * r)
{
	struct mongo_server ret = {NULL, 0};
	char * next;
	char * port;
	
	while (* servers == ' ')
		servers++;
	
	next = (char *) strchr(servers, ',');
	port = (char *) strchr(servers, ':');
	
	if (port < next || (port != NULL && next == NULL))
		ret.port = atoi(port + 1);
	
	if (ret.port == 0 || ret.port > 65535)
		ret.port = MONGO_DEFAULT_PORT;
	
	if (next)
		* next = '\0';
	
	if (port)
		* port = '\0';
	
	ret.host = servers;
	* r = ret;
	
	return ((next) ? (next + 1) : NULL);
}

void db_init (void)
{
	int status;
	struct mongo_server server;
	const char * next;
	
	bson_malloc_func = ALLOCATOR_MALLOC_FN;
	bson_realloc_func = ALLOCATOR_REALLOC_FN;
	bson_free = ALLOCATOR_FREE_FN;
	
	mongo_init(mongo_connection);
	
	if (config.db_replica_set)
	{
		mongo_replset_init(mongo_connection, config.db_replica_set);
		next = config.db_servers;
		
		while ((next = parse_server(next, &server)))
			mongo_replset_add_seed(mongo_connection, server.host, server.port);
		
		status = mongo_replset_connect(mongo_connection);
	}
	else
	{
		parse_server(config.db_servers, &server);
		status = mongo_connect(mongo_connection, server.host, server.port);
	}
	
	if (status != MONGO_OK)
		eerr(-1, "Can't connect to database: %s.", errorcodetomsg(mongo_connection->err));
}
