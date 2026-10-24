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
    uint32_t node_size;
    uint32_t first_leaf;
    hfsp_fork_t allocation, extents, catalog;
} hfsplus_t;

typedef struct {
    uint32_t    parent;     
    uint32_t    cnid;
    uint32_t    size;
    uint8_t     is_dir;
    hfsp_fork_t fork;
    char        name[256];
} hfsp_entry_t;

int hfsplus_mount(hfsplus_t *fs, const partition_t *part);
void hfsplus_ls(hfsplus_t *fs, uint32_t dir_cnid);
int hfsplus_lookup(hfsplus_t *fs, const char *path, hfsp_entry_t *out);
int32_t hfsplus_read(hfsplus_t *fs, const hfsp_entry_t *e, uint32_t offset, void *dst, uint32_t max);