#pragma once

#include <stdint.h>

#define low_16(address) (uint16_t)((address)&0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

int memcmp(const void *__s1, const void *__s2, int __n);
void memcpy(char *source, char *dest, int nbytes);
void memset(void *dest, uint8_t val, uint32_t len);

uint32_t kmalloc(uint32_t size, int align, uint32_t *physical_addr);