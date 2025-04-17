#include "iface/kiss.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


int main( void)
{

	unsigned char input[] = { 'H', 'e', 'l', FESC, 'l', 'o', FEND };
	unsigned char enc_input[] = { FEND, 0x01, 'H', 'e', 'l', FESC, TFESC, 'l', 'o', FESC, TFEND, FEND };
	unsigned char type = 0x01, decoded_type = 0xCC;
	int result;
	size_t encoded_len = kiss_encoded_size(input, sizeof(input)), decoded_len = 0;
	
	unsigned char *encoded = (unsigned char *) malloc(encoded_len);
	if ( encoded == NULL )
	{
		fprintf( stderr, "Error allocating memory.\n");
		return 1;
	}
	
	kiss_encode(input, sizeof(input), type, encoded);

	assert( encoded_len == sizeof(enc_input));
	assert( memcmp( encoded, enc_input, encoded_len) == 0 );
	
	unsigned char *decoded = (unsigned char *) malloc(encoded_len);
	if ( decoded == NULL )
	{
		fprintf( stderr, "Error allocating memory.\n");
		return 1;
	}
	
	result = kiss_decode( input, sizeof(input), &decoded_type, decoded, &decoded_len);
	
	assert( result == 0 );
	assert( decoded_type == 0x01 );
	assert( decoded_len == sizeof( input));
	assert( memcmp( input, decoded, decoded_len) == 0 );
	
	free( encoded);
	free( decoded);
	printf( "All tests passed.\n");
}
