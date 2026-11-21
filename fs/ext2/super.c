#include <quernel/printk.h>
#include <drivers/ata.h>

#include "ext2.h"

int ext2_write_block(ext2_t *fs, uint32_t block, uint16_t *buf)
{
    uint32_t spb = fs->block_size / 512;
    for (uint32_t i = 0; i < spb; i++)
        if (write_partition(&fs->part, block * spb + i, (uint8_t *)buf + i * 512) != 0)
            return -1;
    return 0;
}

int ext2_read_block(ext2_t *fs, uint32_t block, void *buf)
{
    uint32_t spb = fs->block_size / 512;
    for (uint32_t i = 0; i < spb; i++)
        if (read_partition(&fs->part, block * spb + i, (uint8_t *)buf + i * 512) != 0)
            return -1;

    return 0;
}

/**
 * ext2_write_bgdt
 * @fs: the ext2 filesystem
 * @group: the group to save
 *
 * Writes the changes of group to the fs's group descriptor table.
 */
int ext2_write_bgdt(ext2_t *fs, uint32_t group)
{
    uint32_t bgdt_start_blk = (fs->block_size == 1024) ? 2 : 1;
    uint32_t desc_size = sizeof(ext2_group_desc);

    uint32_t target_blk = bgdt_start_blk + ((group * desc_size) / fs->block_size);
    uint32_t offset_in_blk = (group * desc_size) % fs->block_size;

    uint8_t *buf = kmalloc(fs->block_size);
    ext2_read_block(fs, target_blk, buf);

    memcpy(buf + offset_in_blk, (uint8_t *)&fs->bgd[group], desc_size);

    int r = ext2_write_block(fs, target_blk, (uint16_t *)buf);
    kfree(buf);
    return r;
}

/**
 * ext2_write_super_block
 * @fs: the ext2 filesystem
 *
 * Writes the changes of fs->sb to the disk.
 */
int ext2_write_super_block(ext2_t *fs)
{
    uint32_t sb_blk = (fs->block_size == 1024) ? 1 : 0;
    uint32_t offset_in_blk = (fs->block_size == 1024) ? 0 : 1024;

    uint8_t *buf = kmalloc(fs->block_size);
    ext2_read_block(fs, sb_blk, buf);

    memcpy(buf + offset_in_blk, (uint8_t *)&fs->sb, sizeof(ext2_super_block));

    int r = ext2_write_block(fs, sb_blk, (uint16_t *)buf);
    kfree(buf);
    return r;
}

int ext2_mount(ext2_t *fs, partition_t *part)
{
    uint8_t sec[1024] __aligned(2);

    if (read_partition(part, 2, sec) != 0)
    {
        pr_err("Failed to read the superblock!\n");
        return -1;
    }

    const ext2_super_block *sb = (const ext2_super_block *)sec;

    if (sb->signature != EXT2_SIG)
    {
        return -2;
    }

    fs->block_size = 1024u << sb->log_block_size;
    fs->group_count = (sb->blocks_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
    fs->inode_size = (sb->rev_level == 0) ? 128 : sb->s_inode_size;
    fs->part = *part;
    fs->sb = *sb;

    if (fs->group_count > EXT2_MAX_GROUPS)
    {
        pr_err("ext2: too many block groups (%i)\n", fs->group_count);
        return -1;
    }

    uint32_t start_block = fs->sb.first_data_block + 1;
    uint32_t table_bytes = fs->group_count * sizeof(ext2_group_desc);
    uint32_t table_blocks = (table_bytes + fs->block_size - 1) / fs->block_size;

    fs->bgd = kmalloc(table_blocks * fs->block_size + 64);
    memset(fs->bgd, 0xAA, table_blocks * fs->block_size);
    if (!fs->bgd)
        return -1;

    for (uint32_t i = 0; i < table_blocks; i++)
    {
        if (ext2_read_block(fs, start_block + i, (uint8_t *)fs->bgd + i * fs->block_size) != 0)
        {
            kfree(fs->bgd);
            return -1;
        }
    }

    // Pre read the root inode
    ext2_iget(fs, EXT2_ROOT_INO, &(fs->root_inode));

    return 0;
}
