#pragma once
#include <stdint.h>

#define ENOENT        2
#define EIO           5
#define ENOMEM        12
#define ENOTDIR       20
#define EINVAL        22
#define ENAMETOOLONG  36
#define ESTALE        116

static char upper(char c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static inline uint16_t be16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
static inline uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static inline uint64_t be64(const uint8_t *p)
{
    return ((uint64_t)be32(p) << 32) | be32(p + 4);
}