#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "allocators.h"
#include "utils.h"

void allocator_stack_init(Allocator_Stack* allocator, void* backing_buf, size_t backing_buf_len) {
    allocator->buf = (uint8_t*) backing_buf;
    allocator->buf_len = backing_buf_len;
    allocator->prev_offset = 0;
    allocator->curr_offset = 0;
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

    curr_addr = allocator->buf + allocator->curr_offset;
    padding = calc_padding_with_header(curr_addr, (uintptr_t) align, sizeof(Allocator_Stack_Header));
    if (allocator->curr_offset + padding + data_size > allocator->buf_len) {
        // Adding the data at the next aligned address based on the offset, will exceed the buffer length.
        return NULL;
    }

    // offset is updated to be align and "pointing" to the next aligned address of the buffer.
    allocator->prev_offset += allocator->curr_offset;
    allocator->curr_offset += padding;

    // same as `allocator->buf + allocator->curr_offset`, because `allocator->curr_offset` as been aligned previously.
    next_addr = curr_addr + (uintptr_t) padding;

    // Get a pointer backward, this way the header is stored in the padding.
    header              = (Allocator_Stack_Header*) (next_addr - sizeof(Allocator_Stack_Header));
    header->padding     = padding;
    header->prev_offset = allocator->prev_offset;

    allocator->curr_offset += data_size;
    
    return memset((void*) next_addr, 0, data_size);
}

void* allocator_stack_alloc(Allocator_Stack* allocator, size_t data_size) {
    return allocator_stack_alloc_align(allocator, data_size, DEFAULT_ALIGNEMENT);
}

void allocator_stack_free(Allocator_Stack* allocator, void* ptr) {
    if (ptr != NULL) {
        uintptr_t start     = (uintptr_t) allocator->buf;
        uintptr_t end       = start + (uintptr_t) allocator->buf_len;
        uintptr_t curr_addr = (uintptr_t) ptr;
        Allocator_Stack_Header* header;
        size_t prev_offset;

        if (curr_addr < start || curr_addr > end) {
			assert(0 && "Out of bounds memory address passed to stack allocator (free)");
			return;
        }

		if (curr_addr >= start + (uintptr_t)allocator->curr_offset) {
			// Allow double frees
			return;
		}

        header = (Allocator_Stack_Header*) (curr_addr - sizeof(Allocator_Stack_Header));
        prev_offset = (size_t) (curr_addr - (uintptr_t) header->padding - start);

        if (prev_offset != header->prev_offset) {
            assert(0 && "Out of order stack allocator free");
            return;
        }

        allocator->curr_offset = prev_offset;
        allocator->prev_offset = header->prev_offset;
    }
}

void allocator_stack_free_all(Allocator_Stack* allocator) {
    allocator->prev_offset = 0;
    allocator->curr_offset = 0;
}

void* allocator_stack_resize_align(Allocator_Stack* allocator, void* ptr, size_t old_data_size, size_t new_data_size, size_t align) {
    if (ptr == NULL) {
        return allocator_stack_alloc_align(allocator, new_data_size, align);
    } else if (new_data_size == 0) {
        allocator_stack_free(allocator, ptr);
        return NULL;
    } else {
        // TODO: manage when ptr is previous alloc (prev_offset)

        uintptr_t start     = (uintptr_t) allocator->buf;
        uintptr_t end       = (uintptr_t) allocator->buf_len;
        uintptr_t curr_addr = (uintptr_t) ptr;
        size_t min_size     = old_data_size < new_data_size ? old_data_size : new_data_size;
        void* new_ptr;

        if (curr_addr < start || curr_addr > end) {
			assert(0 && "Out of bounds memory address passed to stack allocator (resize)");
			return NULL;
        }

		if (curr_addr >= start + (uintptr_t) allocator->curr_offset) {
			// Treat as a double free
			return NULL;
		}

        if (old_data_size == new_data_size) {
            return ptr;
        }


        new_ptr = allocator_stack_alloc_align(allocator, new_data_size, align);
        memmove(new_ptr, ptr, min_size);
        return new_ptr;
    }
}

void* allocator_stack_resize(Allocator_Stack* allocator, void* ptr, size_t old_data_size, size_t new_data_size) {
    return allocator_stack_resize_align(allocator, ptr, old_data_size, new_data_size, DEFAULT_ALIGNEMENT);
}