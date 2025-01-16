#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "allocators.h"

void allocator_pool_init(Allocator_Pool* allocator, void* backing_buf, size_t backing_buf_len, size_t chunk_size, size_t chunk_align) {
	// Align backing buffer to the specified chunk alignment
	uintptr_t initial_start = (uintptr_t) backing_buf;
	uintptr_t start         = align_forward_uintptr(initial_start, (uintptr_t) chunk_align);
	backing_buf_len        -= (size_t) (start-initial_start);

    // Align chunk size up to the required chunk alignment
	chunk_size = align_forward_size(chunk_size, chunk_align);

	// Assert that the parameters passed are valid
	assert(chunk_size >= sizeof(Allocator_Pool_Free_Node) && "Chunk size is too small");
	assert(backing_buf_len >= chunk_size && "Backing buffer length is smaller than the chunk size");

	// Store the adjusted parameters
	allocator->buf            = (uint8_t*) backing_buf;
	allocator->buf_len        = backing_buf_len;
	allocator->chunk_size     = chunk_size;
	allocator->free_list_head = NULL;

	// Set up the free list for free chunks
	allocator_pool_free_all(allocator);
}

void* allocator_pool_alloc(Allocator_Pool* allocator) {
	Allocator_Pool_Free_Node* free_node = allocator->free_list_head;
	
	if(free_node == NULL) {
		assert(0 && "Pool allocator has no free memory");
		return NULL;
	}

	allocator->free_list_head = allocator->free_list_head->next;

	return memset(free_node, 0, allocator->chunk_size);
}

void allocator_pool_free(Allocator_Pool* allocator, void* ptr) {
	if (ptr == NULL) {
		return;
	}

	Allocator_Pool_Free_Node* free_node;

	void* start = allocator->buf;
	void* end = &allocator->buf[allocator->buf_len];

	if (ptr < start || ptr > end) {
		assert(0 && "Memory is out of bounds of the buffer in this pool");
		return;
	}

	free_node = (Allocator_Pool_Free_Node*) ptr;
	free_node->next = allocator->free_list_head;
	allocator->free_list_head = free_node;
}

void allocator_pool_free_all(Allocator_Pool* allocator) {
    size_t chunk_count = allocator->buf_len / allocator->chunk_size;

	// Set all chunks to be free
    for(size_t i = 0; i < chunk_count; i += 1) {
        void* ptr = &allocator->buf[i * allocator->chunk_size];
        Allocator_Pool_Free_Node* free_node = (Allocator_Pool_Free_Node*) ptr;

		// Push free node onto the free list
        free_node->next = allocator->free_list_head;
        allocator->free_list_head = free_node;
    }
}