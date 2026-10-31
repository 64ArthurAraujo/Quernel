#include <stdint.h>

int memcmp(const void *__s1, const void *__s2, int __n)
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

void memcpy(char *dest, char *src, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        *(dest + i) = *(src + i);
    }
}

void memset(void *dest, uint8_t val, uint32_t len)
{
    uint8_t *temp = (uint8_t *)dest;
    for (; len != 0; len--)
        *temp++ = val;
}

void *memmove(void *dest, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0)
        return dest;

    if (d < s)
    {
        for (uint32_t i = 0; i < n; i++)
            d[i] = s[i];
    }
    else
    {
        for (uint32_t i = n; i != 0; i--)
            d[i - 1] = s[i - 1];
    }

    return dest;
}