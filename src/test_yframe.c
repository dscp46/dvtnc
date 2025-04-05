#include "yframe.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_yframe_ctx_create_and_free() {
    yframe_ctx_t *ctx = yframe_ctx_create(128, NULL, NULL);
    assert(ctx != NULL);
    assert(ctx->frame_buffer != NULL);
    assert(ctx->mtu == 128);
    yframe_ctx_free(ctx);
}

void test_yframe_is_banned_char() {
    assert(yframe_is_banned_char(0x00) == true);
    assert(yframe_is_banned_char(0xE1) == true);
    assert(yframe_is_banned_char(0x50) == false);
}

void test_yframe_encoded_size() {
    unsigned char data[] = {0x01, 0xE1, 0x50};
    size_t size = yframe_encoded_size(data, sizeof(data));
    assert(size == 6);  // Start + encoded bytes + end
}

void test_yframe_encode() {
    unsigned char data[] = {0x01, 0xE1, 0x50};
    void *encoded = NULL;
    size_t encoded_size = 0;
    
    yframe_encode(data, sizeof(data), &encoded, &encoded_size);

    assert(encoded != NULL);
    assert(encoded_size == 6);
    
    unsigned char *enc = (unsigned char *)encoded;
    assert(enc[0] == 0xE1); // Start byte
    assert(enc[1] == 0x01);
    assert(enc[2] == 0x3D); // Escape character
    assert(enc[3] == 0x21);
    assert(enc[4] == 0x50);
    assert(enc[5] == 0xE0); // End byte
    
    free(encoded);
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
    yframe_receive(ctx, frame, sizeof(frame));
    yframe_ctx_free(ctx);
}

int main() {
    test_yframe_ctx_create_and_free();
    test_yframe_is_banned_char();
    test_yframe_encoded_size();
    test_yframe_encode();
    test_yframe_receive();
    printf("All tests passed!\n");
    return 0;
}

