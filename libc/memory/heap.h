#pragma once
#include <stddef.h>
#include <stdint.h>

extern uint8_t kernel_end;

#define TOTAL_MEMORY (1024 * 1024 * 1024)        // 1 GB
#define PAGE_SIZE 4096                           // 4 KB
#define PAGES_AMOUNT (TOTAL_MEMORY / PAGE_SIZE)  // 262,144
#define PAGE_MASK (~(PAGE_SIZE - 1))

typedef struct _heap_block_t
{
    uint32_t size;
    int free;
    struct _heap_block_t *next;
} heap_block_t;
