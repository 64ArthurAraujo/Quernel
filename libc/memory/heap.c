/*
 *  libc/memory/heap.c & heap.h
 *
 *  Implementations taken from MyraOS (https://github.com/dvir-biton/MyraOS/blob/main/system/memory/src/heap/heap.c)
 */
#include <quernel/printk.h>
#include <stddef.h>
#include <stdint.h>
#include <memory.h>

#include "heap.h"
#include "pmm.h"
#include "vmm.h"

#define GET_METADATA_ADDR(block) ((void *)((uintptr_t)block + sizeof(heap_block_t)))

heap_block_t *h_start;
heap_block_t *h_end;

/*
    Function recieves block and the desired size for the block, and splits block into two.
*/
heap_block_t *split_block(heap_block_t *block, size_t size)
{
    if (block->size < size + sizeof(heap_block_t))
        return block; /* too small to split, use the whole block */

    heap_block_t *new_block = (heap_block_t *)((uintptr_t)block + sizeof(heap_block_t) + size);

    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->free = 1;
    new_block->next = block->next;

    if (new_block->next == NULL)
    {
        h_end = new_block;
    }

    block->size = size;
    block->free = 0;
    block->next = new_block;

    return block;
}

void heap_init(uintptr_t start, size_t size)
{
    h_start = (heap_block_t *)start;
    h_start->size = size - sizeof(heap_block_t);
    h_start->free = 1;
    h_start->next = NULL;
    h_end = h_start;
}

void *kmalloc(size_t size)
{
    if (size == 0 || size > HEAP_SIZE)
        return NULL;

    size = (size + 3) & ~3;

    heap_block_t *curr_block = h_start;
    while (curr_block != NULL && (!curr_block->free || curr_block->size < size))
    {
        curr_block = curr_block->next;
    }

    if (curr_block == NULL)
    {
        return NULL;
    }

    if (curr_block->size >= size + sizeof(heap_block_t) + 8)
    {
        curr_block = split_block(curr_block, size);
    }

    return GET_METADATA_ADDR(curr_block);
}

void kfree(void *ptr)
{
    if (ptr == NULL)
        return;

    heap_block_t *target = (heap_block_t *)((uintptr_t)ptr - sizeof(heap_block_t));
    target->free = 1;

    // merge free blocks
    heap_block_t *curr = h_start;
    while (curr != NULL)
    {
        if (curr->free && curr->next != NULL && curr->next->free)
        {
            curr->size += sizeof(heap_block_t) + curr->next->size;
            curr->next = curr->next->next;

            if (curr->next == NULL)
                h_end = curr;

            continue;
        }

        curr = curr->next;
    }
}