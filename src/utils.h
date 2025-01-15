#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/*
  2 * 4 bytes on 32bit machine, 2 * 8 bytes on 64bit machine.
*/ 
#ifndef DEFAULT_ALIGNEMENT
#define DEFAULT_ALIGNEMENT (2 * sizeof(void*))
#endif

/*
  Check if a pointer address is a power of 2.
  The implementation doesn't use the modulo (%) operator but a after approach with bitwise operator.

  Here is the trick : 
  A binary representation of a number wich is a power of 2 has exactly one bit set to '1'. For example:
  - 1 (2^0) = 0001
  - 2 (2^1) = 0010
  - 4 (2^2) = 0100
  - 8 (2^3) = 1000

  When you subtract 1 from a power of 2, all the bits after the single '1' in the binary representation flip. For example:
  - 2 (2^1) = 0001
  - 4 (2^2) = 0011
  - 8 (2^3) = 0111

  The bitwise AND operation produces a '1' only where both bits in the same position are '1'.
  
  So, for example: 
  x = 8 (1000)
  x - 1 = 7 (0111)
  x & (x - 1) = 1000 & 0111 = 0000 = 0 -> 8 is a power of 2

  another example:
  x = 5 (0101)
  x - 1 = 4 (0100)
  x & (x - 1) = 0101 & 0100 = 0100 = 4, 5 is not a power of 2

  Note:
  This implementation consider 0 as a power of 2, which is not correct, but this procedure
  is only used with pointer address inside this project, so 0 should never be passed as argument as 0 is 
  not a valid pointer address, so we save us a check.
*/
bool is_power_of_two(uintptr_t ptr) {
    return (ptr & (ptr -1)) == 0;
}

/*
  Get the next pointer address which follow the desired memory alignement.

  Modern computer architectures will always read memory at its “word size” (4 bytes on a 32 bit machine, 8 bytes on a 64 bit machine).
  If you have an unaligned memory access (on a processor that allows for that), the processor will have to read multiple “words”.
  This means that an unaligned memory access may be much slower than an aligned memory access. 

  to read more about memory alignment, here are some good resources: 
  - https://igoro.com/archive/gallery-of-processor-cache-effects/
  - https://www.rcollins.org/articles/pmbasics/tspec_a1_doc.html

  Now, here is how this procedure works:
  given a pointer and an alignment, we calculate the modulo from the 2. For example, with and alignment of 16 (0x00000010) bytes: 
  ptr % align = 0x7B5FF270 % 0x00000010 = 00000000 -> The ptr is correct 16 bytes aligned.

  ptr % align = 0xDB9FF364 % 0x00000010 = 00000004 -> The ptr is not 16 bytes aligned
  Here the ptr cannot be used directly, a padding/offset has to be added to get the next valid address 16 bytes aligned :
  ptr + (align - modulo) = 0xDB9FF364 + (0x00000010 - 0x00000004) = 0xDB9FF364 + 0x0000000C = 0xDB9FF370 -> this is the next valid address 16 bytes aligned.

*/
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