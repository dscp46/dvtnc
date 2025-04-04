#include "ringbuffer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_ringbuffer_alloc_free(void) {
    ringbuffer_t *buf = ringbuffer_alloc(10);
    assert( buf != NULL );
    assert( buf->buffer != NULL );
    if ( buf->size == 10) {
        printf("test_ringbuffer_alloc_free: PASS\n");
    } else {
        printf("test_ringbuffer_alloc_free: FAIL\n");
    }
    ringbuffer_free(buf);
}

void test_ringbuffer_push_pull(void) {
    ringbuffer_t *buf = ringbuffer_alloc(10);
    char data[] = "hello";
    char dest[6] = {0};

    size_t pushed = ringbuffer_push(data, 5, buf);
    size_t pulled = ringbuffer_pull(dest, 5, buf);
    
    if (pushed == 5 && pulled == 5 && strcmp(dest, "hello") == 0) {
        printf("test_ringbuffer_push_pull: PASS\n");
    } else {
        printf("test_ringbuffer_push_pull: FAIL\n");
    }
    ringbuffer_free(buf);
}

void test_ringbuffer_overflow(void) {
    ringbuffer_t *buf = ringbuffer_alloc(5);
    char data[] = "abcdef";

    size_t pushed = ringbuffer_push(data, 6, buf);
    if (pushed == 4) {
        printf("test_ringbuffer_overflow: PASS\n");
    } else {
        printf("test_ringbuffer_overflow: FAIL\n");
    }
    ringbuffer_free(buf);
}

void test_ringbuffer_empty_pull(void) {
    ringbuffer_t *buf = ringbuffer_alloc(5);
    char dest[5] = {0};

    size_t pulled = ringbuffer_pull(dest, 5, buf);
    if (pulled == 0) {
        printf("test_ringbuffer_empty_pull: PASS\n");
    } else {
        printf("test_ringbuffer_empty_pull: FAIL\n");
    }
    ringbuffer_free(buf);
}

int main() {
    test_ringbuffer_alloc_free();
    test_ringbuffer_push_pull();
    test_ringbuffer_overflow();
    test_ringbuffer_empty_pull();
    return 0;
}
