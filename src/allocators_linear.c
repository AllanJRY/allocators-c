#include <assert.h>
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
    return allocator_linear_alloc_align(allocator, data_size, DEFAULT_ALIGNEMENT);
}

void allocator_linear_free(Allocator_Linear* allocator) {
    allocator->prev_offset = 0;
    allocator->curr_offset = 0;
}

void* allocator_linear_resize_align(Allocator_Linear* allocator, void* old_memory, size_t old_size, size_t new_size, size_t align) {
    uint8_t* old_mem = (uint8_t*) old_memory;

    assert(is_power_of_two(align));

    if (old_mem == NULL || old_size == 0) {
        return allocator_linear_alloc_align(allocator, new_size, align);
    } else if (allocator->buf <= old_mem && old_mem < allocator->buf + allocator->buf_len) {
        if (allocator->buf + allocator->prev_offset == old_mem) {
            allocator->curr_offset = allocator->prev_offset + new_size;
            if (new_size > old_size) {
                // Set to 0 the new memory by default.
                memset(&allocator->buf[allocator->curr_offset], 0, new_size - old_size);
            }

            return old_memory;
        } else {
            void* new_memory = allocator_linear_alloc_align(allocator, new_size, align);
            size_t copy_file = old_size < new_size ? old_size : new_size;
            // Copy old memory to new memory.
            memmove(new_memory, old_memory, old_size);
            return new_memory;
        }
    }  else {
        assert(0 && "Memory is out of bounds of the buffer in this arena");
        return NULL;
    }
}

void* allocator_linear_resize(Allocator_Linear* allocator, void* old_memory, size_t old_size, size_t new_size) {
    return allocator_linear_resize_align(allocator, old_memory, old_size, new_size, DEFAULT_ALIGNEMENT);
}