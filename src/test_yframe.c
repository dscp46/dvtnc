#define YFRAME_INTERNALS
#include "yframe.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_yframe_ctx_create_and_free() {
    yframe_ctx_t *ctx = yframe_ctx_create(128, NULL, NULL);
    assert(ctx != NULL);
    assert(ctx->frame_buffer != NULL);
    assert(ctx->mtu == 128);
    ctx->free(ctx);
}

void test_yframe_is_banned_char() {
    assert(yframe_is_banned_char(0x00) == true);
    assert(yframe_is_banned_char(0xE1) == true);
    assert(yframe_is_banned_char(0x50) == false);
}

void test_yframe_encode() {
    unsigned char data[] = {0x01, 0xE1, 0x50};
    UT_string *encoded = NULL;
    utstring_new(encoded);

    yframe_encode(data, sizeof(data), encoded);

    assert(utstring_len(encoded) == 6);
    
    unsigned char *enc = (unsigned char *)utstring_body(encoded);
    assert(enc[0] == 0xE1); // Start byte
    assert(enc[1] == 0x01);
    assert(enc[2] == 0x3D); // Escape character
    assert(enc[3] == 0x21);
    assert(enc[4] == 0x50);
    assert(enc[5] == 0xE0); // End byte
    
    utstring_free(encoded);
}

void test_received_frame( void *args)
{
    yframe_cb_args_t *result = (yframe_cb_args_t*)args;
    unsigned char *buf = (unsigned char *)result->buf;
    assert(result->n == 2);
    assert(buf[0] == 0x01);
    assert(buf[1] == 0x50);
}

void test_yframe_receive() {
    yframe_ctx_t *ctx = yframe_ctx_create(128, &test_received_frame, NULL);
    unsigned char frame[] = {0xE1, 0x01, 0x50, 0xE0};
    ctx->receive(ctx, frame, sizeof(frame));
    ctx->free(ctx);
}

int main() {
    test_yframe_ctx_create_and_free();
    test_yframe_is_banned_char();
    test_yframe_encode();
    test_yframe_receive();
    printf("All tests passed!\n");
    return 0;
}

