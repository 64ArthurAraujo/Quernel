#pragma once
#include <drivers/ata.h>
#include <stdint.h>

// HFS PLUS
typedef struct
{
    uint32_t start, count;
} hfsp_extent_t;

typedef struct
{
    uint64_t size;
    uint32_t total_blocks;
    hfsp_extent_t ext[8];
} hfsp_fork_t;

typedef struct
{
    partition_t part;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t sectors_per_block;
    uint8_t is_hfsx;
    hfsp_fork_t allocation, extents, catalog;
} hfsplus_t;

int hfsplus_mount(hfsplus_t *fs, const partition_t *part);