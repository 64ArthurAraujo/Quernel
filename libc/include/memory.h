#pragma once

#include <stdint.h>

#define low_16(address) (uint16_t)((address)&0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

void mem_copy(char *source, char *dest, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);

uint32_t kmalloc(uint32_t size, int align, uint32_t *physical_addr);