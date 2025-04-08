#ifndef __APP_H
#define __APP_H

#include "serial.h"
#include "yframe.h"

#include <pthread.h>
#include <signal.h>
#include <stdint.h>

typedef struct app_settings_t {
	char *serial_fname;
	int verbose;
	serial_t serial;
	
	uint16_t kiss_port;
	pthread_t kiss_srv_thread;
	
	// FIXME: Active client (will be replaced by a hashmap after the mono-client PoC works and the AX.25 DSE is implemented)
	int client_fd;
	
	yframe_ctx_t *yframe_rx_ctx;
} app_settings_t;

#ifndef __APP_C
extern app_settings_t* _settings;
#else	/* __APP_C */
app_settings_t* _settings;
#endif	/* __APP_C */

void print_usage( char *prog_name);

void parse_args( int argc, char *argv[], app_settings_t *settings);

void sigint_action( int signum, siginfo_t *info, void *ctx);

void set_signals( void);

#endif	/* __APP_H */
