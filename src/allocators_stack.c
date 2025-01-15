#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "allocators.h"
#include "utils.h"

void allocator_stack_init(Allocator_Stack* allocator, void* backing_buf, size_t backing_buf_len) {
    allocator->buf = (uint8_t*) backing_buf;
    allocator->buf_len = backing_buf_len;
    allocator->offset = 0;
}

static size_t calc_padding_with_header(uintptr_t ptr, uintptr_t align, size_t header_size) {
    assert(is_power_of_two(align));

    uintptr_t modulo, padding, needed_space;

    modulo       = ptr & (align - 1);
    padding      = 0;
    needed_space = 0;

    if (modulo != 0) { // Same as align_foward from utils.h
        padding = align - modulo;
    }

    needed_space = (uintptr_t) header_size;
    if (padding < needed_space) {
        needed_space -= padding;

        if(needed_space & (align - 1) != 0) {
            padding += align * (1 + (needed_space / align));
        } else {
            padding += align * (needed_space / align);
        }
    }

    return (size_t) padding;
}

void* allocator_stack_alloc_align(Allocator_Stack* allocator, size_t data_size, size_t align) {
    assert(is_power_of_two(align));
    
    uintptr_t curr_addr, next_addr;
    size_t padding;
    Allocator_Stack_Header* header;

	if (align > 128) {
		// As the padding is 8 bits (1 byte), the largest alignment that can
		// be used is 128 bytes
		align = 128;
	}

    curr_addr = allocator->buf + allocator->offset;
    padding = calc_padding_with_header(curr_addr, (uintptr_t) align, sizeof(Allocator_Stack_Header));
    if (allocator->offset + padding + data_size > allocator->buf_len) {
        // Adding the data at the next aligned address based on the offset, will exceed the buffer length.
        return NULL;
    }

    // offset is updated to be align and "pointing" to the next aligned address of the buffer.
    allocator->offset += padding;

    // same as `allocator->buf + allocator->offset`, because `allocator->offset` as been aligned previously.
    next_addr = curr_addr + (uintptr_t) padding;

    // Get a pointer backward, this way the header is stored in the padding.
    header = (Allocator_Stack_Header*) (next_addr - sizeof(Allocator_Stack_Header));
    header->padding = (uint8_t) padding;

    allocator->offset += data_size;
    
    return memset((void*) next_addr, 0, data_size);
}

void* allocator_stack_alloc(Allocator_Stack* allocator, size_t data_size) {
    return allocator_stack_alloc_align(allocator, data_size, DEFAULT_ALIGNEMENT);
}

void allocator_stack_free(Allocator_Stack* allocator, void* ptr) {
    if (ptr != NULL) {
        uintptr_t start, end, curr_addr;
        Allocator_Stack_Header* header;
        size_t prev_offset;

        start     = (uintptr_t) allocator->buf;
        end       = start + (uintptr_t) allocator->buf_len;
        curr_addr = (uintptr_t) ptr;

        if (!(start <= curr_addr && curr_addr < end)) {
			assert(0 && "Out of bounds memory address passed to stack allocator (free)");
			return;
        }

		if (curr_addr >= start + (uintptr_t)allocator->offset) {
			// Allow double frees
			return;
		}

        header = (Allocator_Stack_Header*) (curr_addr - sizeof(Allocator_Stack_Header));
        prev_offset = (size_t) (curr_addr - (uintptr_t) header->padding - start);

        allocator->offset = prev_offset;
    }
}
