#ifndef ALLOCATORS_H
#define ALLOCATORS_H

/**
 * A linear allocator, also known as an arena or region-based allocator, manages memory allocations
 * sequentially within a single continuous block of memory. Deallocation is performed in one step 
 * by resetting the allocator, freeing the entire buffer rather than individual allocations.
 *
 * This structure encapsulates the state and configuration of the linear allocator.
 *
 * Members:
 * - `uint8_t* buf`:
 *   Pointer to the backing buffer where memory allocations are managed.
 * - `size_t buf_len`:
 *   Total size of the backing buffer, in bytes.
 * - `size_t prev_offset`:
 *   Offset to the previous allocation, useful for resizing or reverting the last allocation.
 * - `size_t curr_offset`:
 *   Current offset within the buffer, marking the position for the next allocation.
 *
 * ### Key Characteristics:
 * - Memory is allocated sequentially, ensuring low overhead and fast allocation times.
 * - No support for freeing individual allocations; the entire buffer is reset at once.
 * - Designed for scenarios where allocation patterns are predictable and temporary, 
 *   such as transient data or frame-based memory management in games or simulations.
 */
typedef struct Allocator_Linear {
    uint8_t* buf;         // Pointer to the backing buffer
    size_t   buf_len;     // Total length of the backing buffer, in bytes
    size_t   prev_offset; // Offset to the previous allocation
    size_t   curr_offset; // Offset for the next allocation
} Allocator_Linear;

/**
 * Initializes a linear allocator with a specified backing buffer.
 *
 * This function allows the user to provide a backing buffer, granting flexibility 
 * to determine the memory's lifetime (e.g., stack or heap). The allocator will 
 * manage memory allocations within the bounds of the provided buffer.
 *
 * @param allocator       Pointer to the `Allocator_Linear` structure to initialize.
 * @param backing_buf     Pointer to the memory buffer that the allocator will manage.
 * @param backing_buf_len Size of the backing buffer, in bytes.
 *
 * ### Notes:
 * - The backing buffer must remain valid for the duration of the allocator's use.
 * - Users can control the buffer's memory allocation source (e.g., stack for short-lived allocators 
 *   or heap for longer-lived allocators).
 * - Ensure that `backing_buf_len` is sufficient for the anticipated allocations to prevent buffer overflows.
 */
void allocator_linear_init(Allocator_Linear* allocator, void* backing_buf, size_t backing_buf_len);

/**
 * Allocates memory from a linear allocator with the specified alignment.
 * Ensures that each new allocation adheres to the given alignment requirement
 * by adding padding if necessary.
 *
 * @param allocator   Pointer to the linear allocator managing the memory buffer.
 * @param data_size   Size of the data to allocate, in bytes.
 * @param data_align  Alignment requirement, in bytes. Must be a power of two.
 *
 * @return A pointer to the allocated memory block, or NULL if allocation fails
 *         due to insufficient space.
 *
 * ### Allocation Behavior:
 * 1. If the current pointer of the allocator is already aligned, no padding is added.
 * 2. If the current pointer is not aligned for the requested data, padding is added to meet the alignment requirement.
 * 3. The allocator's current pointer is incremented by the data size (and any padding) after each allocation.
 *
 * ### Example with an 8-Byte Alignment:
 * - **First allocation (16 bytes):**
 *   - If the allocator starts aligned, no padding is added.
 *   - The allocator pointer is moved 16 bytes forward, maintaining 8-byte alignment.
 * 
 * - **Second allocation (6 bytes):**
 *   - No padding is needed if the previous allocation ended at an 8-byte-aligned address.
 *   - The allocator pointer is incremented by 6 bytes, potentially breaking alignment.
 * 
 * - **Third allocation (10 bytes):**
 *   - A 2-byte padding is added to realign the pointer to an 8-byte boundary.
 *   - The pointer is then incremented by 10 bytes, breaking alignment again.
 *
 * ### Efficiency Considerations:
 * To maximize buffer utilization and minimize padding overhead:
 * - Allocate larger, highly aligned data first.
 * - Group allocations with similar alignment requirements together.
 * Excessive padding due to poor ordering can reduce performance and waste buffer space.
 */
void* allocator_linear_alloc_align(Allocator_Linear* allocator, size_t data_size, size_t data_align);

/**
 * Allocates memory from the linear allocator with a default alignment.
 *
 * This function is a convenience wrapper around `allocator_linear_alloc_align`, 
 * automatically using a predefined default alignment. It simplifies memory allocation 
 * when specific alignment requirements are not needed.
 *
 * @param allocator   Pointer to the `Allocator_Linear` structure managing the memory buffer.
 * @param data_size   Size of the memory block to allocate, in bytes.
 *
 * @return A pointer to the allocated memory block, or NULL if the allocation fails 
 *         due to insufficient space in the allocator's buffer.
 *
 * ### Notes:
 * - The `DEFAULT_ALIGNEMENT` constant defines the alignment applied to the allocation. 
 *   Ensure this value meets your application's typical requirements.
 * - If precise alignment is necessary, use `allocator_linear_alloc_align` instead.
 * - This function moves the allocator's current offset forward by the allocated size 
 *   (plus any padding for alignment), reducing the available space in the buffer.
 */
void* allocator_linear_alloc(Allocator_Linear* allocator, size_t data_size);

/**
 * Resets the linear allocator, freeing all allocated memory.
 *
 * This function resets the allocator's state by clearing its offsets. It does not 
 * deallocate the backing buffer itself, as the allocator does not own the buffer's 
 * memory. All previously allocated memory becomes invalid after calling this function.
 *
 * @param allocator   Pointer to the `Allocator_Linear` structure to reset.
 *
 * ### Notes:
 * - This operation effectively "frees" all allocations made from the allocator 
 *   without the need to track or free them individually.
 * - The backing buffer remains allocated and must be managed by the user.
 * - This function is ideal for resetting temporary or frame-based allocations.
 */
void allocator_linear_free(Allocator_Linear* allocator);

/**
 * Resizes a previously allocated memory block in a linear allocator, with specified alignment.
 *
 * This function handles resizing of memory in three scenarios:
 * 1. If no memory is provided (`old_memory` is `NULL` or `old_size` is `0`), it allocates a new block.
 * 2. If the block to resize is the most recently allocated one, it resizes it in place.
 * 3. Otherwise, it allocates a new block, copies the data, and returns the new block.
 *
 * @param allocator   Pointer to the `Allocator_Linear` managing the memory.
 * @param old_memory  Pointer to the memory block to resize. Can be `NULL` to allocate new memory.
 * @param old_size    The size of the original memory block, in bytes.
 * @param new_size    The desired new size of the memory block, in bytes.
 * @param align       Alignment requirement for the memory block. Must be a power of two.
 *
 * @return A pointer to the resized memory block, or NULL if allocation fails or 
 *         `old_memory` is out of the allocator's buffer bounds.
 *
 * ### Behavior:
 * - **New Allocation**:
 *   - If `old_memory` is `NULL` or `old_size` is `0`, the function allocates a new block 
 *     using the provided alignment and size.
 * - **In-Place Resizing**:
 *   - If `old_memory` corresponds to the most recent allocation (tracked by `prev_offset`):
 *     - Expands the block in place if the new size is larger, zeroing out the extra memory.
 *     - Shrinks the block by adjusting the current offset if the new size is smaller.
 * - **Block Relocation**:
 *   - If the block cannot be resized in place, a new block is allocated:
 *     - Copies data from the old block to the new one (up to the smaller of the old and new sizes).
 *     - Returns a pointer to the new memory block.
 * - **Error Handling**:
 *   - If `old_memory` is outside the allocator's buffer bounds, the function asserts and returns `NULL`.
 *
 * ### Notes:
 * - The alignment (`align`) must be a power of two. This is validated with an `assert`.
 * - The function ensures that additional memory in an expanded block is zeroed by default.
 * - Relocated blocks require sufficient free space in the allocator's buffer for the new allocation.
 *
 * ### Example Usage:
 * ```c
 * void* memory = allocator_linear_alloc_align(&allocator, 32, 16);
 * memory = allocator_linear_resize_align(&allocator, memory, 32, 64, 16);
 * ```
 */
void* allocator_linear_resize_align(Allocator_Linear* allocator, void* old_memory, size_t old_size, size_t new_size, size_t align);

/**
 * Resizes a previously allocated memory block in a linear allocator, using a default alignment.
 *
 * This function is a simplified wrapper around `allocator_linear_resize_align`, 
 * automatically using a predefined default alignment. It handles resizing of memory 
 * blocks while ensuring alignment consistent with the default configuration.
 *
 * @param allocator   Pointer to the `Allocator_Linear` managing the memory.
 * @param old_memory  Pointer to the memory block to resize. Can be `NULL` to allocate new memory.
 * @param old_size    The size of the original memory block, in bytes.
 * @param new_size    The desired new size of the memory block, in bytes.
 *
 * @return A pointer to the resized memory block, or NULL if allocation fails or 
 *         `old_memory` is out of the allocator's buffer bounds.
 *
 * ### Behavior:
 * - **New Allocation**:
 *   - If `old_memory` is `NULL` or `old_size` is `0`, a new memory block of size `new_size` 
 *     is allocated with the default alignment.
 * - **In-Place Resizing**:
 *   - If `old_memory` corresponds to the most recent allocation:
 *     - Expands the block in place if `new_size` is larger, zeroing out the extra memory.
 *     - Shrinks the block by adjusting the current offset if `new_size` is smaller.
 * - **Block Relocation**:
 *   - If resizing cannot be done in place, a new block is allocated:
 *     - Copies data from the old block to the new one (up to the smaller of the old and new sizes).
 *     - Returns a pointer to the new memory block.
 * - **Error Handling**:
 *   - If `old_memory` is outside the allocator's buffer bounds, the function asserts and returns `NULL`.
 *
 * ### Notes:
 * - The `DEFAULT_ALIGNEMENT` constant defines the alignment applied to the allocation. Ensure 
 *   this value meets your application's requirements.
 * - If precise alignment is required, use `allocator_linear_resize_align` instead.
 * - The function does not deallocate memory; previously allocated memory becomes invalid 
 *   when a new block is created.
 *
 * ### Example Usage:
 * ```c
 * void* memory = allocator_linear_alloc(&allocator, 32);
 * memory = allocator_linear_resize(&allocator, memory, 32, 64);
 * ```
 */
void* allocator_linear_resize(Allocator_Linear* allocator, void* old_memory, size_t old_size, size_t new_size);

typedef struct Allocator_Stack_Header {
    uint8_t padding; // Padding of the previous allocation (in bytes), added before the header to have the new allocation aligned correctly.
} Allocator_Stack_Header;

typedef struct Allocator_Stack {
    uint8_t* buf;     // Pointer to the backing buffer
    size_t   buf_len; // Total length of the backing buffer, in bytes
    size_t   offset;  // Offset to the previous allocation
} Allocator_Stack;

void allocator_stack_init(Allocator_Stack* allocator, void* backing_buf, size_t backing_buf_len);

void* allocator_stack_alloc_align(Allocator_Stack* allocator, size_t data_size, size_t align);

void* allocator_stack_alloc(Allocator_Stack* allocator, size_t data_size);

#endif