#ifndef ALLOCATORS_H
#define ALLOCATORS_H

/*
    TODO: doc linear allocator
*/

typedef struct Allocator_Linear {
    uint8_t* buf;
    size_t   buf_len;
    size_t   prev_offset;
    size_t   curr_offset;
} Allocator_Linear;

void allocator_linear_init(Allocator_Linear* allocator, void* backing_buf, size_t backing_buf_len);

void* allocator_linear_alloc_align(Allocator_Linear* allocator, size_t data_size, size_t data_align);

void* allocator_linear_alloc(Allocator_Linear* allocator, size_t data_size);

void allocator_linear_free(Allocator_Linear* allocator);

void allocator_linear_resize_align(Allocator_Linear* allocator, void* old_buf, size_t old_size, size_t new_size, size_t align);

void allocator_linear_resize(Allocator_Linear* allocator, void* old_buf, size_t old_size, size_t new_size);

#endif