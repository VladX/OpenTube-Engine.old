#include "common_functions.h"
#include "core_server.h"
#include "core_config.h"

#define COPYRIGHT "Copyright (C) 2011 VladX (http://vladx.net/); Bugs to <vvladxx@gmail.com>"


void parse_args (char ** args, int n)
{
	if (n > 1 && (strcmp("-c", args[1]) != 0 || n < 3))
		eerr(1, "Usage: %s [-c /path/to/configuration/file]\n\n" COPYRIGHT, args[0]);
	
	if (n >= 3)
		load_config(args[2]);
	else
		load_config(CONFIG_PATH);
}

int main (int argc, char ** argv)
{
	parse_args(argv, argc);
	init(* argv);
	
	return 0;
}
