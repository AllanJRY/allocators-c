#include <stdint.h>
#include <stdbool.h>

#ifndef DEFAULT_ALIGNEMENT
#define DEFAULT_ALIGNEMENT (2 * sizeof(void*))
#endif

bool is_power_of_two(uintptr_t ptr) {
    return (ptr & (ptr -1)) == 0;
}

uintptr_t align_forward(uintptr_t ptr, size_t align) {
    assert(is_power_of_two(align));

    uintptr_t p      = ptr;
    uintptr_t a      = (uintptr_t) align;
    uintptr_t modulo = p & (a - 1); // Fast modulo as 'a' is a power of 2.

    if (modulo != 0) {
		// If 'p' address is not aligned, push the address to the
		// next value which is aligned
		p += a - modulo;
    }

    return p;
}

typedef struct Allocator_Linear {
    uint8_t* buf;
    size_t   buf_len;
    size_t   prev_offset;
    size_t   curr_offset;
} Allocator_Linear;

void allocator_linear_init(Allocator_Linear* allocator, void* backing_buf, size_t backing_buf_len) {
    allocator->buf = (uint8_t*) backing_buf;
    allocator->buf_len = backing_buf_len;
    allocator->prev_offset = 0;
    allocator->curr_offset = 0;
}

void* allocator_linear_alloc(Allocator_Linear* allocator, size_t data_size, size_t data_align) {
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


int main(void) {
    uint8_t test = '1';
    return 0;
}