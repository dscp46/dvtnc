#ifndef __APP_H
#define __APP_H

#include <signal.h>
#include <stdint.h>

typedef struct app_settings_t {
	char *serial_fname;
	uint16_t kiss_port;
	int verbose;
} app_settings_t;


void print_usage( char *prog_name);

void parse_args( int argc, char *argv[], app_settings_t *settings);

void sigint_action( int signum, siginfo_t *info, void *ctx);

void set_signals( void);

#endif	/* __APP_H */
