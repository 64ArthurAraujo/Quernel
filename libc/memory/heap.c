/*
 *  libc/memory/heap.c & heap.h
 *
 *  Implementations taken from MyraOS (https://github.com/dvir-biton/MyraOS/blob/main/system/memory/src/heap/heap.c)
 */
#include <quernel/printk.h>
#include <stddef.h>
#include <stdint.h>

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
    if (block->size > size)
    {
        pr_err("split block failed: block_size > size");
        return NULL;
    }

    heap_block_t *new_block = (heap_block_t *)((uintptr_t)block + sizeof(heap_block_t) + size);

    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->free = true;
    new_block->next = block->next;

    if (new_block->next == NULL)
    {
        h_end = new_block;
    }

    block->size = size;
    block->free = false;
    block->next = new_block;

    return block;
}

void heap_init(uintptr_t start, size_t size)
{
    for (size_t offset = 0; offset < size; offset += PAGE_SIZE)
    {
        uintptr_t virt = start + offset;
        uintptr_t phys = (uintptr_t)pmm_alloc_page();
        vmm_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE);
    }
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
        uintptr_t natural_addr = (uintptr_t)h_end + h_end->size + sizeof(heap_block_t);

        uintptr_t aligned_addr = (natural_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        heap_block_t *new_block = (heap_block_t *)aligned_addr;

        size_t total_needed = size + sizeof(heap_block_t);
        size_t pages_needed = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;

        for (size_t i = 0; i < pages_needed; i++)
        {
            uintptr_t phys = (uintptr_t)pmm_alloc_page();

            if (phys == 0)
            {
                for (size_t j = 0; j < i; j++)
                {
                    vmm_unmap_page(aligned_addr + j * PAGE_SIZE);
                }
                return NULL;
            }
            vmm_map_page(aligned_addr + i * PAGE_SIZE, phys, PAGE_PRESENT | PAGE_WRITE);
        }

        uintptr_t gap_size = aligned_addr - natural_addr;
        if (gap_size > 0)
            h_end->size += gap_size;

        new_block->size = (pages_needed * PAGE_SIZE) - sizeof(heap_block_t);
        new_block->free = 1;
        new_block->next = NULL;

        h_end->next = new_block;
        h_end = new_block;
        curr_block = new_block;
    }

    if (curr_block->size > size * sizeof(heap_block_t) + 8)
    {
        curr_block = split_block(curr_block, size);
    }

    curr_block->free = 0;
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