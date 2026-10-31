#pragma once
#include <stdint.h>
#include <stddef.h>

#define low_16(address) (uint16_t)((address)&0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

int memcmp(const void *__s1, const void *__s2, int __n);
void memcpy(char *dest, char *src, int n);
void memset(void *dest, uint8_t val, uint32_t len);
void *memmove(void *dest, const void *src, uint32_t n);

/* libc/memory/heap.c */
void heap_init(uintptr_t start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
