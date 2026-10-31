#pragma once
#include <stddef.h>
#include <stdint.h>

#include "heap.h"

#define BITMAP_SIZE PAGES_AMOUNT / 8
#define BIOS_PART 512

void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *addr);
size_t pmm_get_free_page_amount(void);
