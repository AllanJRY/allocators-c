#include <stdint.h>
#include <stdio.h>

#include "src/allocators.h"

#define BACK_BUF_LEN 1024

int main(void) {
    uint8_t back_buf[BACK_BUF_LEN];

    Allocator_Linear allocator;
    allocator_linear_init(&allocator, back_buf, BACK_BUF_LEN);
    allocator_linear_alloc(&allocator, 32); // Will not cause padding
    allocator_linear_alloc(&allocator, 36); // Will cause padding
    allocator_linear_alloc(&allocator, 24); // Will cause padding
    allocator_linear_free(&allocator);

    return 0;
}