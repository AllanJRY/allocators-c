#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

bool is_power_of_two(uintptr_t x) {
    return (x & (x -1)) == 0;
}

uintptr_t align_forward_uintptr(uintptr_t ptr, uintptr_t align) {
  assert(is_power_of_two(align));

  uintptr_t modulo = ptr & (align - 1); // Fast modulo as 'align' is a power of 2.

  uintptr_t aligned_ptr = ptr;
  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    aligned_ptr += align - modulo;
  }

  return aligned_ptr;
}

size_t align_forward_size(size_t ptr, size_t align) {
  assert(is_power_of_two((uintptr_t) align));

  size_t modulo = ptr & (align - 1); // Fast modulo as 'align' is a power of 2.

  size_t aligned_ptr = ptr;
  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    aligned_ptr += align - modulo;
  }

  return aligned_ptr;
}