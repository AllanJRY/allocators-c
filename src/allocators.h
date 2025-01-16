#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include "utils.h"

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

// TODO: Allocator_Linear_Temp

/**
 * Metadata for managing allocations in a stack-based allocator.
 *
 * The `Allocator_Stack_Header` structure is used to store metadata about a specific 
 * allocation in the stack allocator. It helps manage the stack-like behavior of the allocator.
 *
 * Members:
 * - `prev_offset`: The offset within the buffer to the previous allocation. 
 *                  This allows the allocator to backtrack and free the last allocation.
 * - `padding`: The padding (in bytes) added before this header to ensure the alignment of the current allocation.
 *
 * ### Notes:
 * - This header is typically stored in memory just before the allocated data block it describes.
 * - The padding value ensures that subsequent allocations meet the alignment requirements.
 */
typedef struct Allocator_Stack_Header {
    size_t prev_offset; // Offset to the previous allocation
    size_t padding;     // Padding of the previous allocation (in bytes), added before the header to have the new allocation aligned correctly.
} Allocator_Stack_Header;

/**
 * A stack-based memory allocator for efficient, temporary memory management.
 *
 * The `Allocator_Stack` struct implements a stack-like allocator, where memory is allocated and 
 * deallocated in a last-in, first-out (LIFO) order. This approach is more flexible than an linear/arena allocator, 
 * allowing individual deallocation of the most recent allocation without impacting earlier ones.
 *
 * Members:
 * - `buf`: Pointer to the backing buffer that provides the memory storage for allocations.
 * - `buf_len`: Total size of the backing buffer, in bytes.
 * - `prev_offset`: Offset to the most recently allocated block, allowing the allocator to track 
 *                  and backtrack allocations.
 * - `curr_offset`: Offset to the next available memory address in the buffer for new allocations.
 *
 * ### Behavior:
 * - **Allocation**: Memory is allocated sequentially from the buffer. The `curr_offset` is updated 
 *   to reflect the newly allocated memory block.
 * - **Deallocation**: Memory can be freed by reverting the `curr_offset` to the `prev_offset` of 
 *   the last allocation, effectively "popping" it off the stack.
 * - **Efficiency**: This allocator is efficient for temporary memory usage patterns that follow a 
 *   LIFO structure. However, it does not support random deallocation of older blocks.
 *
 * ### Notes:
 * - The stack allocator operates entirely within the bounds of the `buf` backing buffer.
 * - It is the user's responsibility to ensure that allocations fit within the remaining buffer space.
 * - The stack allocator does not reclaim memory for allocations that are not the most recent, adhering to its LIFO principle.
 *
 * ### Example Usage:
 * ```c
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, buffer_size);
 * 
 * void* mem1 = allocator_stack_alloc(&stack, 32); // Allocate 32 bytes with default alignment
 * void* mem2 = allocator_stack_alloc(&stack, 16); // Allocate 16 bytes with default alignment
 * 
 * allocator_stack_free(&stack, mem2); // Free the most recent allocation (mem2)
 * allocator_stack_free(&stack, mem1); // Free the next allocation (mem1)
 * ```
 */
typedef struct Allocator_Stack {
    uint8_t* buf;         // Pointer to the backing buffer
    size_t   buf_len;     // Total length of the backing buffer, in bytes
    size_t   prev_offset; // Offset to the previous allocation
    size_t   curr_offset; // Offset to the next available allocation address
} Allocator_Stack;

/**
 * Initializes a stack-based allocator with a given backing buffer.
 *
 * This function sets up an `Allocator_Stack` structure to manage memory allocations
 * from a user-provided backing buffer. The backing buffer allows the user to control
 * where the memory is allocated (e.g., stack or heap) and its size.
 *
 * @param allocator       Pointer to the `Allocator_Stack` to initialize.
 * @param backing_buf     Pointer to the backing buffer used for allocations. Must not be `NULL`.
 * @param backing_buf_len The size of the backing buffer in bytes.
 *
 * ### Behavior:
 * - The `buf` pointer in the allocator is set to the provided `backing_buf`.
 * - The total buffer length is recorded in `buf_len`.
 * - Both `prev_offset` and `curr_offset` are initialized to `0`, indicating that 
 *   no memory has been allocated yet.
 *
 * ### Notes:
 * - The provided `backing_buf` should be properly aligned for the intended usage. 
 *   Misaligned buffers may lead to undefined behavior.
 * - The user is responsible for ensuring that the `backing_buf` remains valid for 
 *   the lifetime of the allocator.
 * - This function does not allocate or manage the backing buffer itself; it simply 
 *   initializes the allocator to operate on the given memory.
 *
 * ### Example Usage:
 * ```c
 * uint8_t buffer[1024]; // Backing buffer
 * Allocator_Stack stack;
 * 
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 * 
 * void* mem1 = allocator_stack_alloc_align(&stack, 64, 8); // Allocate 64 bytes with 8-byte alignment
 * ```
 */
void allocator_stack_init(Allocator_Stack* allocator, void* backing_buf, size_t backing_buf_len);

/**
 * Allocates aligned memory from a stack-based allocator with header metadata.
 *
 * This function allocates memory from an `Allocator_Stack` instance while ensuring that 
 * the memory address is aligned to the specified alignment. Metadata about the allocation, 
 * such as the padding and the previous offset, is stored in the padding space before the 
 * allocated memory block.
 *
 * @param allocator   Pointer to the `Allocator_Stack` instance managing the memory.
 * @param data_size   The size of the memory block to allocate, in bytes.
 * @param align       The alignment requirement for the allocation, which must be a power of two.
 *                    If the alignment exceeds 128, it is capped at 128 due to padding constraints.
 *
 * @return A pointer to the newly allocated and zero-initialized memory block, or `NULL` if 
 *         there is insufficient space in the buffer to accommodate the allocation.
 *
 * ### Behavior:
 * - **Alignment**: Ensures the allocated memory address satisfies the specified alignment.
 *   - Uses `calc_padding_with_header` to calculate the required padding, considering alignment and header size.
 * - **Header Placement**: Stores metadata (`Allocator_Stack_Header`) within the padding area, just 
 *   before the aligned memory address.
 * - **Buffer Bounds**: If the allocation exceeds the available space in the buffer, the function 
 *   returns `NULL` without modifying the allocator state.
 * - **State Updates**: Updates the allocator's `curr_offset` and `prev_offset` to reflect the new allocation.
 * - **Zero Initialization**: The allocated memory block is zero-initialized for convenience.
 *
 * ### Implementation Notes:
 * - The maximum supported alignment is 128 bytes due to the use of an 8-bit padding value in the header.
 * - The function ensures alignment is a power of two using an assertion (`assert(is_power_of_two(align))`).
 * - The allocation metadata (`padding` and `prev_offset`) enables efficient deallocation in a stack-like manner.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 *
 * // Allocate 64 bytes with 16-byte alignment
 * void* block1 = allocator_stack_alloc_align(&stack, 64, 16);
 *
 * // Allocate 128 bytes with 32-byte alignment
 * void* block2 = allocator_stack_alloc_align(&stack, 128, 32);
 *
 * if (!block1 || !block2) {
 *     printf("Allocation failed due to insufficient space.\n");
 * }
 * ```
 *
 * ### Error Handling:
 * - If the allocation exceeds the buffer size, the function returns `NULL` without modifying the allocator.
 *
 * ### Limitations:
 * - Alignments greater than 128 bytes are automatically capped at 128.
 */
void* allocator_stack_alloc_align(Allocator_Stack* allocator, size_t data_size, size_t align);

/**
 * Allocates memory from a stack-based allocator with default alignment.
 *
 * This function is a convenience wrapper around `allocator_stack_alloc_align`. It allocates 
 * memory of the specified size (`data_size`) from the `Allocator_Stack` using the default alignment.
 *
 * @param allocator   Pointer to the `Allocator_Stack` instance managing the memory.
 * @param data_size   The size of the memory block to allocate, in bytes.
 *
 * @return A pointer to the newly allocated and zero-initialized memory block, or `NULL` if 
 *         there is insufficient space in the buffer to accommodate the allocation.
 *
 * ### Behavior:
 * - Uses the default alignment (`DEFAULT_ALIGNMENT`) for the allocation.
 * - Delegates the allocation logic to `allocator_stack_alloc_align`, ensuring that alignment and 
 *   memory constraints are properly handled.
 * - Returns a pointer to the allocated memory block, which is zero-initialized for convenience.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 *
 * // Allocate 64 bytes using the default alignment
 * void* block = allocator_stack_alloc(&stack, 64);
 *
 * if (!block) {
 *     printf("Allocation failed due to insufficient space.\n");
 * }
 * ```
 *
 * ### Error Handling:
 * - If the allocation exceeds the buffer size or alignment requirements cannot be met, 
 *   the function returns `NULL` without modifying the allocator.
 *
 * ### Notes:
 * - This function simplifies memory allocation when specific alignment is not required.
 * - The default alignment is defined by the `DEFAULT_ALIGNMENT` macro. */
void* allocator_stack_alloc(Allocator_Stack* allocator, size_t data_size);

/**
 * Frees memory previously allocated from a stack-based allocator.
 *
 * This function deallocates memory from an `Allocator_Stack` instance. It ensures that memory 
 * is freed in a last-in, first-out (LIFO) order, maintaining the integrity of the stack-based 
 * allocation system. The function also performs boundary and order checks to catch improper usage.
 *
 * @param allocator   Pointer to the `Allocator_Stack` instance managing the memory.
 * @param ptr         Pointer to the memory block to free. If `NULL`, the function does nothing.
 *
 * ### Behavior:
 * - **Null Pointer Handling**: If `ptr` is `NULL`, the function exits without any action.
 * - **Bounds Check**: Ensures the `ptr` lies within the valid range of the stack allocator's buffer.
 *   - If the address is out of bounds, the function asserts with an error message and exits.
 * - **Double Free Check**: Prevents freeing memory that is beyond the current offset, effectively 
 *   allowing "double frees" without altering the allocator state.
 * - **Order Check**: Verifies that memory is freed in LIFO order. If an out-of-order free is detected, 
 *   the function asserts with an error message and exits.
 * - **State Updates**: Adjusts the allocator's `curr_offset` and `prev_offset` to reflect the deallocation.
 *
 * ### Preconditions:
 * - The pointer `ptr` must have been allocated by the same `Allocator_Stack` instance.
 * - Memory must be freed in reverse order of allocation (LIFO principle).
 *
 * ### Error Handling:
 * - **Out of Bounds**: If `ptr` is outside the allocator's buffer range, the function asserts 
 *   with the message: `"Out of bounds memory address passed to stack allocator (free)"`.
 * - **Out of Order Free**: If memory is not freed in the correct LIFO order, the function asserts 
 *   with the message: `"Out of order stack allocator free"`.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 *
 * // Allocate memory
 * void* block1 = allocator_stack_alloc(&stack, 64);
 * void* block2 = allocator_stack_alloc(&stack, 128);
 *
 * // Free memory in reverse order of allocation
 * allocator_stack_free(&stack, block2);
 * allocator_stack_free(&stack, block1);
 *
 * // Improper free (out of order) will trigger an assertion
 * // allocator_stack_free(&stack, block1); // Uncommenting will assert
 * ```
 *
 * ### Notes:
 * - This function enforces the stack-like allocation order by verifying the `prev_offset` stored 
 *   in the `Allocator_Stack_Header` against the calculated offset.
 * - It is designed to detect and prevent misuse, such as out-of-bounds or out-of-order freeing.
 *
 * ### Limitations:
 * - Only supports stack-based allocation patterns (LIFO). Any other freeing order results in an error.
 */
void allocator_stack_free(Allocator_Stack* allocator, void* ptr);

/**
 * Frees all memory allocated by the stack-based allocator.
 *
 * This function resets the `Allocator_Stack` to its initial state, effectively marking all 
 * memory in the buffer as free. It does not modify the contents of the buffer, but subsequent 
 * allocations will overwrite the previously allocated data.
 *
 * @param allocator   Pointer to the `Allocator_Stack` instance to reset.
 *
 * ### Behavior:
 * - Sets both `prev_offset` and `curr_offset` to `0`, resetting the allocator's state.
 * - Does not perform any deallocation of the underlying buffer, as it is managed externally.
 * - All pointers to previously allocated memory become invalid after this operation.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 *
 * // Allocate memory
 * void* block1 = allocator_stack_alloc(&stack, 64);
 * void* block2 = allocator_stack_alloc(&stack, 128);
 *
 * // Reset the stack allocator
 * allocator_stack_free_all(&stack);
 *
 * // After resetting, all previous allocations are invalid
 * // Any new allocation will start from the beginning of the buffer
 * void* new_block = allocator_stack_alloc(&stack, 256);
 * ```
 *
 * ### Notes:
 * - Use this function to quickly free all allocated memory when it is no longer needed, without 
 *   having to free individual allocations in reverse order.
 * - This operation is non-destructive to the buffer's contents but invalidates all previous 
 *   allocations.
 * - Suitable for scenarios where the entire buffer's memory can be reclaimed at once.
 *
 * ### Limitations:
 * - Does not provide any checks or warnings for invalidating existing pointers; the user is 
 *   responsible for ensuring that pointers to previous allocations are no longer used.
 */
void allocator_stack_free_all(Allocator_Stack* allocator);

/**
 * Resizes an allocated block in the stack-based allocator with alignment.
 *
 * This function attempts to resize a memory block managed by the stack allocator. If the block 
 * is the most recent allocation, the resize operation is performed in place. Otherwise, a new 
 * block is allocated, and the existing data is copied over. The memory block can also be freed 
 * by passing a `new_data_size` of `0`.
 *
 * @param allocator       Pointer to the `Allocator_Stack` instance managing the memory.
 * @param ptr             Pointer to the existing memory block to resize. Can be `NULL`.
 * @param old_data_size   The current size of the memory block (in bytes).
 * @param new_data_size   The desired new size of the memory block (in bytes).
 * @param align           Alignment of the memory block. Must be a power of two.
 * 
 * @return
 * - Pointer to the resized memory block.
 * - If the block is resized in place, the same pointer is returned.
 * - If the block is moved, a new pointer is returned with the resized data.
 * - Returns `NULL` if the memory block is freed or if the resize operation fails.
 *
 * ### Behavior:
 * 1. **Null Pointer Case**:
 *    - If `ptr` is `NULL`, a new block of size `new_data_size` is allocated with the given alignment.
 * 2. **In-Place Resize**:
 *    - If `ptr` points to the most recently allocated block, the block is resized in place.
 *    - If the block grows, the new memory is zero-initialized.
 * 3. **Free Memory**:
 *    - If `new_data_size` is `0`, the block is freed, and `NULL` is returned.
 * 4. **Relocation**:
 *    - If the block is not the most recent allocation, a new block is allocated.
 *    - The existing data (up to the minimum of the old and new sizes) is copied to the new block.
 * 5. **Bounds Check**:
 *    - Ensures the pointer lies within the buffer's bounds. An out-of-bounds pointer triggers an 
 *      assertion failure.
 *    - Prevents double-free scenarios by validating the pointer against the current allocation state.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 * 
 * // Allocate memory
 * void* block = allocator_stack_alloc_align(&stack, 64, 16);
 * 
 * // Resize the block to a larger size
 * block = allocator_stack_resize_align(&stack, block, 64, 128, 16);
 * 
 * // Resize the block to a smaller size
 * block = allocator_stack_resize_align(&stack, block, 128, 32, 16);
 * 
 * // Free the block
 * allocator_stack_resize_align(&stack, block, 32, 0, 16);
 * ```
 *
 * ### Notes:
 * - This function is most efficient when resizing the most recent allocation in the stack.
 * - For non-recent allocations, a new memory block is allocated, and the original data is copied, 
 *   which incurs additional overhead.
 * - If `align` is greater than `128`, it will be clamped to `128` due to internal padding limits.
 *
 * ### Assertions:
 * - The `align` parameter must be a power of two.
 * - Ensures the pointer is within the bounds of the allocator's buffer.
 *
 * ### Limitations:
 * - Resizing non-recent allocations requires a full copy of the data, which may be inefficient for 
 *   large allocations.
 * - Does not provide guarantees for performance in scenarios with frequent non-recent resizes.
 */
void* allocator_stack_resize_align(Allocator_Stack* allocator, void* ptr, size_t old_data_size, size_t new_data_size, size_t align);

/**
 * Resizes an allocated block in the stack-based allocator with default alignment.
 *
 * This function resizes a memory block managed by the stack allocator, using the default alignment. 
 * If the block is the most recent allocation, the resize operation is performed in place. Otherwise, 
 * a new block is allocated, and the existing data is copied over. Passing a `new_data_size` of `0` 
 * frees the memory block.
 *
 * @param allocator       Pointer to the `Allocator_Stack` instance managing the memory.
 * @param ptr             Pointer to the existing memory block to resize. Can be `NULL`.
 * @param old_data_size   The current size of the memory block (in bytes).
 * @param new_data_size   The desired new size of the memory block (in bytes).
 * 
 * @return
 * - Pointer to the resized memory block.
 * - If the block is resized in place, the same pointer is returned.
 * - If the block is moved, a new pointer is returned with the resized data.
 * - Returns `NULL` if the memory block is freed or if the resize operation fails.
 *
 * ### Behavior:
 * 1. **Null Pointer Case**:
 *    - If `ptr` is `NULL`, a new block of size `new_data_size` is allocated with the default alignment.
 * 2. **In-Place Resize**:
 *    - If `ptr` points to the most recently allocated block, the block is resized in place.
 *    - If the block grows, the new memory is zero-initialized.
 * 3. **Free Memory**:
 *    - If `new_data_size` is `0`, the block is freed, and `NULL` is returned.
 * 4. **Relocation**:
 *    - If the block is not the most recent allocation, a new block is allocated.
 *    - The existing data (up to the minimum of the old and new sizes) is copied to the new block.
 *
 * ### Example:
 * ```c
 * uint8_t buffer[1024];
 * Allocator_Stack stack;
 * allocator_stack_init(&stack, buffer, sizeof(buffer));
 * 
 * // Allocate memory
 * void* block = allocator_stack_alloc(&stack, 64);
 * 
 * // Resize the block to a larger size
 * block = allocator_stack_resize(&stack, block, 64, 128);
 * 
 * // Resize the block to a smaller size
 * block = allocator_stack_resize(&stack, block, 128, 32);
 * 
 * // Free the block
 * allocator_stack_resize(&stack, block, 32, 0);
 * ```
 *
 * ### Notes:
 * - This function wraps `allocator_stack_resize_align` and uses the default alignment.
 * - It is most efficient when resizing the most recent allocation in the stack.
 * - For non-recent allocations, a new memory block is allocated, and the original data is copied, 
 *   which incurs additional overhead.
 *
 * ### Limitations:
 * - Resizing non-recent allocations requires a full copy of the data, which may be inefficient for 
 *   large allocations.
 * - Does not allow control over alignment. For custom alignment, use `allocator_stack_resize_align`.
 */
void* allocator_stack_resize(Allocator_Stack* allocator, void* ptr, size_t old_data_size, size_t new_data_size);

typedef struct Allocator_Pool_Free_Node Allocator_Pool_Free_Node;
struct Allocator_Pool_Free_Node {
    Allocator_Pool_Free_Node *next;
};

typedef struct Allocator_Pool {
    uint8_t* buf;
    size_t   buf_len;
    size_t   chunk_size;

    Allocator_Pool_Free_Node* free_list_head; // the free list, behaves like LinkedList.
} Allocator_Pool;

void allocator_pool_init(Allocator_Pool* allocator, void* backing_buf, size_t backing_buf_len, size_t chunk_size, size_t size_chunk_align);

void allocator_pool_free_all(Allocator_Pool* allocator);

#endif