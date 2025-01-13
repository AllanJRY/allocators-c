#ifndef ALLOCATORS_H
#define ALLOCATORS_H

/*
    TODO: doc linear allocator
*/

typedef struct Allocator_Linear Allocator_Linear;

void allocator_linear_init(Allocator_Linear* allocator, void* backing_buf, size_t backing_buf_len);

void* allocator_linear_alloc(Allocator_Linear* allocator, size_t data_size, size_t data_align);

#endif