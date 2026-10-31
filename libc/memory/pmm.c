/*
 *  libc/memory/pmm.c & pmm.h
 *
 *  Implementations taken from MyraOS (https://github.com/dvir-biton/MyraOS/blob/main/system/memory/src/pmm/pmm.c)
 */
#include <quernel/printk.h>
#include <stddef.h>
#include <stdint.h>
#include <memory.h>

#include "pmm.h"

static uint8_t bitmap[BITMAP_SIZE];
static size_t free_page_count;

extern uint32_t _kernel_end;
#define KERNEL_VIRT_BASE 0xC0000000u

void set_bit(uint8_t *bitmap, const size_t bit)
{
    size_t bitmap_index = bit / 8;
    size_t bit_index = bit % 8;

    uint8_t mask = 1 << bit_index;

    bitmap[bitmap_index] |= mask;
}

void clear_bit(uint8_t *bitmap, const size_t bit)
{
    size_t bitmap_index = bit / 8;
    size_t bit_index = bit % 8;

    uint8_t mask = ~(1 << bit_index);

    bitmap[bitmap_index] &= mask;
}

bool test_bit(uint8_t *bitmap, const size_t bit)
{
    size_t bitmap_index = bit / 8;
    size_t bit_index = bit % 8;

    return (bitmap[bitmap_index] >> bit_index) & 1;
}

void pmm_init(void)
{
    for (size_t i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0;

    uint32_t kend = (uint32_t)&_kernel_end;
    size_t reserved = (kend + PAGE_SIZE - 1) / PAGE_SIZE;
    if (reserved < BIOS_PART)
        reserved = BIOS_PART;

    for (size_t p = 0; p < reserved; p++)
        set_bit(bitmap, p);

    size_t heap_first = 0x01000000 / PAGE_SIZE;
    size_t heap_pages = (4 * 1024 * 1024) / PAGE_SIZE;
    for (size_t p = heap_first; p < heap_first + heap_pages; p++)
        set_bit(bitmap, p);

    free_page_count = PAGES_AMOUNT - reserved - heap_pages;
}

void *pmm_alloc_page(void)
{
    if (free_page_count == 0)
        goto oom;

    for (size_t i = 0; i < BITMAP_SIZE; i++)
    {
        if (bitmap[i] < 0xFF)
        {
            for (size_t j = 0; j < 8; j++)
            {
                size_t bit_index = i * 8 + j;

                if (!test_bit(bitmap, bit_index))
                {
                    set_bit(bitmap, bit_index);
                    free_page_count--;
                    return (void *)((uintptr_t)bit_index * PAGE_SIZE);
                }
            }
        }
    }

oom:
    pr_err("quernel is out of memory!\n");
    return NULL;
}

void pmm_free_page(void *addr)
{
    uintptr_t bitmap_index = (uintptr_t)addr / PAGE_SIZE;

    if (bitmap_index >= PAGES_AMOUNT || bitmap_index < BIOS_PART)
    {
        return;
    }

    if (test_bit(bitmap, bitmap_index))
    {
        free_page_count++;
        clear_bit(bitmap, bitmap_index);
    }
}

size_t pmm_get_free_page_amount(void)
{
    return free_page_count;
}