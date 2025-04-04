#include "app.h"
#include "common.h"
#include "kiss.h"
#include "ringbuffer.h"


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char *argv[], unused char *envp[])
{
	app_settings_t settings;
	parse_args( argc, argv, &settings);
	
	if ( settings.serial_fname == NULL || settings.kiss_port == 0 )
	{
		print_usage( argv[0]);
		exit(0);
	}
	
	settings.serial = serial_open(  settings.serial_fname, B9600);
	kiss_start_server( &settings);
	
	_settings = &settings;
	set_signals();
	
	printf( "Application started.\n");

	// TODO: Builtin CLI
	while (true)
		sleep(1);

	return 0;
}
