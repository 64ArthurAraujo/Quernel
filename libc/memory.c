#include <stdint.h>

int mem_cmp(const void *__s1, const void *__s2, int __n)
{
    const uint8_t *a = (const uint8_t *)__s1;
    const uint8_t *b = (const uint8_t *)__s2;

    for (int i = 0; i < __n; i++)
    {
        if (a[i] != b[i])
            return a[i] - b[i];
    }

    return 0;
}

void mem_copy(char *source, char *dest, int nbytes)
{
    int i;

    for (i = 0; i < nbytes; i++)
    {
        *(dest + i) = *(source + i);
    }
}

void memory_set(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *temp = (uint8_t *)dest;
    for (; len != 0; len--)
        *temp++ = val;
}

uint32_t free_mem_address = 0x10000;

uint32_t kmalloc(uint32_t size, int align, uint32_t *physical_addr)
{
    if (align == 1 && (free_mem_address & 0xFFFFF000))
    {
        free_mem_address &= 0xFFFFF000;
        free_mem_address += 0x1000;
    }

    if (physical_addr)
    {
        *physical_addr = free_mem_address;
    }

    uint32_t ret = free_mem_address;
    free_mem_address += size;

    return ret;
}