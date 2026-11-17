#include <quernel/printk.h>
#include <stdint.h>
#include <string.h>

#include "ext2.h"

static uint32_t blocks_in_group(ext2_t *fs, uint32_t g)
{
    uint32_t bpg = fs->sb.blocks_per_group;
    uint32_t start = fs->sb.first_data_block + g * bpg;
    uint32_t left = fs->sb.blocks_count - start;
    return left < bpg ? left : bpg;
}

uint32_t ext2_alloc_block(ext2_t *fs, uint32_t ino)
{
    uint32_t bs = fs->block_size;
    uint32_t n_groups = fs->group_count;

    if (fs->sb.free_blocks_count == 0)
        return 0;

    uint32_t *bitmap = (uint32_t *)kmalloc(bs);
    if (!bitmap)
        return 0;

    uint32_t start_group = 0;

    if (ino > 0)
        start_group = ((ino - 1) / fs->sb.inodes_per_group) % n_groups;

    for (uint32_t k = 0; k < n_groups; k++)
    {
        uint32_t group = (start_group + k) % n_groups;

        if (fs->bgd[group].free_blocks_count == 0)
            continue;

        if (ext2_read_block(fs, fs->bgd[group].block_bitmap, bitmap) != 0)
            continue;

        uint32_t n_bits = blocks_in_group(fs, group);

        for (uint32_t b = 0; b < n_bits; b++)
        {
            if (bitmap[b / 8] & (1 << (b % 8)))
                continue;

            bitmap[b / 8] |= (1 << (b % 8));

            if (ext2_write_block(fs, fs->bgd[group].block_bitmap, (uint16_t *)bitmap) != 0)
                goto fail;

            kfree(bitmap);
            return fs->sb.first_data_block + group * fs->sb.blocks_per_group + b;
        }
    }

fail:
    kfree(bitmap);
    return 0;
}

int ext2_free_block(ext2_t *fs, uint32_t blk)
{
    uint32_t bs = fs->block_size;

    if (blk < fs->sb.first_data_block || blk >= fs->sb.blocks_count)
        return -EINVAL;

    uint32_t rel = blk - fs->sb.first_data_block;
    uint32_t g = rel / fs->sb.blocks_per_group;
    uint32_t b = rel % fs->sb.blocks_per_group;

    uint8_t *bitmap = (uint8_t *)kmalloc(bs);
    if (!bitmap)
        return -ENOMEM;

    int r = 0;

    if (ext2_read_block(fs, fs->bgd[g].block_bitmap, bitmap) != 0)
    {
        r = -EIO;
        goto out;
    }

    if (!(bitmap[b / 8] & (1 << (b % 8))))
    {
        r = -EINVAL;
        goto out;
    }

    bitmap[b / 8] &= ~(1 << (b % 8));

    if (ext2_write_block(fs, fs->bgd[g].block_bitmap, (uint16_t *)bitmap) != 0)
    {
        r = -EIO;
        goto out;
    }

    fs->bgd[g].free_blocks_count++;
    fs->sb.free_blocks_count++;

    if (ext2_write_bgdt(fs, g) != 0 || ext2_write_super_block(fs) != 0)
        r = -EIO;

out:
    kfree(bitmap);
    return r;
}

int ext2_set_indirect_blocks(ext2_t *fs, ext2_inode *inode, uint32_t *slot, int level, uint64_t idx, uint32_t phys, uint32_t goal)
{
    uint32_t bs = fs->block_size;
    uint32_t pointers_per_blk = bs / 4;
    int created = 0;
    int r;

    uint32_t *tbl = (uint32_t *)kmalloc(bs);
    if (!tbl)
        return -ENOMEM;

    if (*slot == 0)
    {
        uint32_t blk = ext2_alloc_block(fs, goal);
        if (blk == 0)
        {
            kfree(tbl);
            return -ENOSPC;
        }

        memset(tbl, 0, bs);

        if (ext2_write_block(fs, blk, (uint16_t *)tbl) != 0)
        {
            ext2_free_block(fs, blk);
            kfree(tbl);
            return -EIO;
        }
        *slot = blk;
        inode->i_blocks += bs / 512;
        created = 1;
    }
    else if (ext2_read_block(fs, *slot, tbl) != 0)
    {
        kfree(tbl);
        return -EIO;
    }

    uint64_t span = 1;
    for (int l = 1; l < level; l++)
        span *= pointers_per_blk;

    // this is dumb and probably will be the root of problems later but it compiles :)
    uint32_t i = (uint32_t)(((uint32_t)idx) / ((uint32_t)span));
    if (i >= pointers_per_blk)
    {
        r = -EINVAL;
        goto fail;
    }

    if (level == 1)
    {
        tbl[i] = phys;
        r = (ext2_write_block(fs, *slot, (uint16_t *)tbl) != 0) ? -EIO : 0;
    }
    else
    {
        uint32_t before = tbl[i];
        // this too is dumb but it compiles :)
        r = ext2_set_indirect_blocks(fs, inode, &tbl[i], level - 1, ((uint32_t)idx) % ((uint32_t)span), phys, goal);

        if (r == 0 && tbl[i] != before && ext2_write_block(fs, *slot, (uint16_t *)tbl) != 0)
            r = -EIO;
    }

    if (r == 0)
    {
        kfree(tbl);
        return 0;
    }

fail:
    if (created)
    {
        ext2_free_block(fs, *slot);
        *slot = 0;
        inode->i_blocks -= bs / 512;
    }
    kfree(tbl);
    return r;
}

int ext2_set_bmap(ext2_t *fs, ext2_inode *inode, uint32_t n, uint32_t phys)
{
    uint64_t P = fs->block_size / 4;
    uint64_t idx = n;

    if (idx < 12)
    {
        inode->i_block[idx] = phys;
        return 0;
    }
    idx -= 12;

    if (idx < P)
        return ext2_set_indirect_blocks(fs, inode, &inode->i_block[12], 1, idx, phys, 0);
    idx -= P;

    if (idx < P * P)
        return ext2_set_indirect_blocks(fs, inode, &inode->i_block[13], 2, idx, phys, 0);
    idx -= P * P;

    if (idx < P * P * P)
        return ext2_set_indirect_blocks(fs, inode, &inode->i_block[14], 3, idx, phys, 0);

    return -EFBIG;
}