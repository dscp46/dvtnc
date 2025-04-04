#include "app.h"
#include "common.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


void print_usage( char *prog_name)
{
	printf("Usage: %s [options]\n", prog_name);
	printf("Options:\n");
	printf("  -h, --help                    Show this help message\n");
	printf("  -k <port>, --kissport <port>  Set the KISS port number\n");
	printf("  -s <file>, --serial <file>    Specify serial line\n");
	printf("  -v, --verbose                 Enable verbose mode\n");
}

void parse_args( int argc, char *argv[], app_settings_t *settings)
{
	int opt;
	settings->serial_fname = NULL;
	settings->verbose = 0;
	settings->kiss_port = 0;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"kissport", required_argument, 0, 'k'},
		{"serial", required_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "hk:s:v", long_options, NULL)) != -1)
	{
		switch (opt)
		{
		case 'h':
			print_usage( argv[0]);
			exit(0);

		case 'k':
			settings->kiss_port = (uint16_t) strtoul( optarg, NULL, 10);
			if ( errno != 0 )
			{
				printf( "Invalid port number.\n");
				print_usage( argv[0]);
			}
			break;
			
		case 's':
			settings->serial_fname = optarg;
			printf("Serial line: %s\n", settings->serial_fname);
			break;

		case 'v':
			settings->verbose = 1;
			printf("Verbose mode enabled\n");
			break;

		default:
			fprintf(stderr, "Usage: %s [-h] [-s <file>] [-v]\n", argv[0]);
			exit(1);
		}
	}
}

void sigint_action( int signum, unused siginfo_t *info, unused void *ctx)
{
	if ( signum != SIGINT )
		return;

	printf( "Caught SIGINT, quitting...\n");
	exit(0);
}

void set_signals( void)
{
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigint_action;
	sigaction(SIGINT, &sa, NULL);
}
