#include <stdint.h>
#include <stdio.h>

#include "allocators.h"

#define BACK_BUF_LEN 1024

typedef struct Position { uint32_t x, y; } Position;

void demo_allocator_linear() {
    printf("# Linear Allocator\n\n");
    uint8_t back_buf[BACK_BUF_LEN];
    Allocator_Linear allocator;
    allocator_linear_init(&allocator, back_buf, BACK_BUF_LEN);

    // allocator_linear_alloc(&allocator, 32); // Will not cause padding
    // allocator_linear_alloc(&allocator, 36); // Will cause padding
    // allocator_linear_alloc(&allocator, 24); // Will cause padding

    Position* pos_1 = (Position*) allocator_linear_alloc(&allocator, sizeof(Position));
    pos_1->x = 10;
    pos_1->y = 20;
    printf("Position 1 (%08llx): x=%d y=%d\n", (uintptr_t)pos_1, pos_1->x, pos_1->y);

    Position* pos_2 = (Position*) allocator_linear_alloc(&allocator, sizeof(Position));
    pos_2->x = 33;
    pos_2->y = 33;
    printf("Position 2 (%08llx): x=%d y=%d\n", (uintptr_t)pos_2, pos_2->x, pos_2->y);

    allocator_linear_free(&allocator);
    printf("Allocator freed\n");

    // Should have same address as pos_1, because the allocator has been freed.
    Position* pos_3 = (Position*) allocator_linear_alloc(&allocator, sizeof(Position));
    pos_3->x = 52;
    pos_3->y = 89;
    printf("Position 3 (%08llx): x=%d y=%d\n", (uintptr_t)pos_3, pos_3->x, pos_3->y);
    printf("\n\n");
}

void demo_allocator_stack() {
    printf("# Stack Allocator\n\n");
    uint8_t back_buf[BACK_BUF_LEN];

    Allocator_Stack allocator;
    allocator_stack_init(&allocator, back_buf, BACK_BUF_LEN);

    Position* pos_1 = (Position*) allocator_stack_alloc(&allocator, sizeof(Position));
    pos_1->x = 10;
    pos_1->y = 20;
    printf("Position 1 (%08llx): x=%d y=%d\n", (uintptr_t)pos_1, pos_1->x, pos_1->y);

    Position* pos_2 = (Position*) allocator_stack_alloc(&allocator, sizeof(Position));
    pos_2->x = 90;
    pos_2->y = 100;
    printf("Position 2 (%08llx): x=%d y=%d\n", (uintptr_t)pos_2, pos_2->x, pos_2->y);

    allocator_stack_free(&allocator, pos_2);
    printf("Position 2 (%08llx) freed\n", (uintptr_t)pos_2);

    // Should have same address as pos_2, because pos_2 has been freed.
    Position* pos_3 = (Position*) allocator_stack_alloc(&allocator, sizeof(Position));
    pos_3->x = 2;
    pos_3->y = 56;
    printf("Position 3 (%08llx): x=%d y=%d\n", (uintptr_t)pos_3, pos_3->x, pos_3->y);

    allocator_stack_free_all(&allocator);
    printf("\n\n");
}

void demo_allocator_pool() {
    printf("# Pool Allocator\n\n");
    uint8_t back_buf[sizeof(Position) * 10];

    Allocator_Pool allocator;
    allocator_pool_init(&allocator, back_buf, BACK_BUF_LEN, sizeof(Position), 32);

    Position* pos_1 = (Position*) allocator_pool_alloc(&allocator);
    pos_1->x = 10;
    pos_1->y = 20;
    printf("Position 1 (%08llx): x=%d y=%d\n", (uintptr_t)pos_1, pos_1->x, pos_1->y);

    Position* pos_2 = (Position*) allocator_pool_alloc(&allocator);
    pos_2->x = 90;
    pos_2->y = 100;
    printf("Position 2 (%08llx): x=%d y=%d\n", (uintptr_t)pos_2, pos_2->x, pos_2->y);

    allocator_pool_free(&allocator, pos_2);
    printf("Position 2 (%08llx) freed\n", (uintptr_t)pos_2);

    // Should have same address as pos_2, because pos_2 has been freed.
    Position* pos_3 = (Position*) allocator_pool_alloc(&allocator);
    pos_3->x = 2;
    pos_3->y = 56;
    printf("Position 3 (%08llx): x=%d y=%d\n", (uintptr_t)pos_3, pos_3->x, pos_3->y);

    allocator_pool_free_all(&allocator);
    printf("\n\n");
}

int main(void) {
    printf("\n");
    demo_allocator_linear();
    demo_allocator_stack();
    demo_allocator_pool();
    return 0;
}