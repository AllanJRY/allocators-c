#include <stdint.h>
#include <string.h>
#include "allocators.h"
#include "utils.h"

void allocator_linear_init(Allocator_Linear* allocator, void* backing_buf, size_t backing_buf_len) {
    allocator->buf         = (uint8_t*) backing_buf;
    allocator->buf_len     = backing_buf_len;
    allocator->prev_offset = 0;
    allocator->curr_offset = 0;
}

void* allocator_linear_alloc_align(Allocator_Linear* allocator, size_t data_size, size_t data_align) {
    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t) allocator->buf + (uintptr_t) allocator->curr_offset;
    uintptr_t offset = align_forward(curr_ptr, data_align);
    offset -= (uintptr_t) allocator->buf; // Change to relative offset

    // Check to see if the backing memory has space left
    if (offset + data_size <= allocator->buf_len) {
        void* ptr = &allocator->buf[offset];
        allocator->prev_offset = offset;
        allocator->curr_offset = offset + data_size;
        memset(ptr, 0, data_size);
        return ptr;
    }

	// Return NULL if the arena is out of memory (or handle differently)
	return NULL;
}

void* allocator_linear_alloc(Allocator_Linear* allocator, size_t data_size) {
    // TODO
}

void allocator_linear_free(Allocator_Linear* allocator) {
    // TODO
}

void allocator_linear_resize_align(Allocator_Linear* allocator, void* old_buf, size_t old_size, size_t new_size, size_t align) {
    // TODO
}

void allocator_linear_resize(Allocator_Linear* allocator, void* old_buf, size_t old_size, size_t new_size) {
    // TODO
}