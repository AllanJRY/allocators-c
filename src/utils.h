#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

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

#endif