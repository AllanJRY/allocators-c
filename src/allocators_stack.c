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
        return NULL;
    }
    allocator->offset += padding;

    next_addr = curr_addr + (uintptr_t) padding;
    header = (Allocator_Stack_Header*) (next_addr - sizeof(Allocator_Stack_Header));
    header->padding = (uint8_t) padding;

    allocator->offset += data_size;
    
    return memset((void*) next_addr, 0, data_size);

}

void* allocator_stack_alloc(Allocator_Stack* allocator, size_t data_size) {
    return allocator_stack_alloc_align(allocator, data_size, DEFAULT_ALIGNEMENT);
}