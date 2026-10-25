#include <drivers/ata.h>
#include <quernel/printk.h>
#include <memory.h>

#include "ext2.h"

/**
 *	ext2_block_to_path - parse the block number into array of offsets
 *
 *	To store the locations of file's data ext2 uses a data structure common
 *	for UNIX filesystems - tree of pointers anchored in the inode, with
 *	data blocks at leaves and indirect blocks in intermediate nodes.
 *	This function translates the block number into path in that tree -
 *	return value is the path length and @offsets[n] is the offset of
 *	pointer to (n+1)th node in the nth one. If @block is out of range
 *	(negative or too large) warning is printed and zero returned.
 */
static int ext2_block_to_path(ext2_t *fs, uint32_t i_block, uint32_t offsets[4])
{
    uint32_t ptrs = fs->block_size / 4;
    uint32_t ptrs_bits = fs->sb.log_block_size + 8;

    const uint32_t direct_blocks = EXT2_NDIR_BLOCKS;
    const uint32_t indirect_blocks = ptrs;
    const uint32_t double_blocks = 1u << (ptrs_bits * 2);

    int n = 0;

    if (i_block < direct_blocks)
    {
        offsets[n++] = i_block;
    }
    else if ((i_block -= direct_blocks) < indirect_blocks)
    {
        offsets[n++] = EXT2_IND_BLOCK;
        offsets[n++] = i_block;
    }
    else if ((i_block -= indirect_blocks) < double_blocks)
    {
        offsets[n++] = EXT2_DIND_BLOCK;
        offsets[n++] = i_block >> ptrs_bits;
        offsets[n++] = i_block & (ptrs - 1);
    }
    else if (((i_block -= double_blocks) >> (ptrs_bits * 2)) < ptrs)
    {
        offsets[n++] = EXT2_TIND_BLOCK;
        offsets[n++] = i_block >> (ptrs_bits * 2);
        offsets[n++] = (i_block >> ptrs_bits) & (ptrs - 1);
        offsets[n++] = i_block & (ptrs - 1);
    }
    else
    {
        pr_err("ext2 warning: %i block is too big\n", i_block);
    }

    return n;
}

static int ext2_read_block(ext2_t *fs, uint32_t block, void *buf)
{
    uint32_t spb = fs->block_size / 512;
    for (uint32_t i = 0; i < spb; i++)
        if (read_partition(&fs->part, block * spb + i, (uint8_t *)buf + i * 512) != 0)
            return -1;

    return 0;
}

int64_t ext2_bmap(ext2_t *fs, const ext2_inode *ino, uint32_t i_block)
{
    uint32_t offsets[4];

    int depth = ext2_block_to_path(fs, i_block, offsets);

    if (depth == 0)
        return -1;

    uint32_t blk = ino->i_block[offsets[0]];

    if (depth == 1)
        return blk; // direct block

    uint8_t buf[fs->block_size];

    if (!buf)
        return -1;

    for (int i = 1; i < depth; i++)
    {
        if (blk == 0)
            return 0;

        if (ext2_read_block(fs, blk, buf) != 0)
        {
            return -1;
        }

        blk = buf[offsets[i]];
    }

    return blk;
}

int ext2_read_inode(ext2_t *fs, uint32_t inode, ext2_inode *out)
{
    if (inode == 0 || inode > fs->sb.inodes_count)
        return -1;

    uint32_t group = (inode - 1) / fs->sb.inodes_per_group;
    uint32_t index = (inode - 1) % fs->sb.inodes_per_group;

    uint32_t byte_offset = index * fs->inode_size;

    uint32_t block_in_tbl = byte_offset / fs->block_size;
    uint32_t offset_in_blk = byte_offset % fs->block_size;

    uint32_t abs_block = fs->bgd[group].inode_table + block_in_tbl;
    uint32_t bgdt_block = (fs->block_size == 1024) ? 2 : 1;
    uint32_t group_count = (fs->sb.blocks_count + fs->sb.blocks_per_group - 1) / fs->sb.blocks_per_group;

    uint32_t block = fs->bgd[group].inode_table + (byte_offset / fs->block_size);

    uint32_t spb = fs->block_size / 512;

    uint8_t buf[fs->block_size];

    if (!buf)
        return -1;

    if (ext2_read_block(fs, block, buf) != 0)
    {
        pr_err("Partition read failed!");
        return -1;
    }

    memcpy((char *)out, buf + offset_in_blk, sizeof(ext2_inode));

    return 0;
}

static int ext2_check_dir_block(ext2_t *fs, const uint8_t *buf, uint32_t limit)
{
    uint32_t offs = 0;
    uint32_t rec_len = 0;

    for (; offs + EXT2_DIR_REC_LEN(1) <= limit; offs += rec_len)
    {
        const ext2_dir_entry *p = (const ext2_dir_entry *)(buf + offs);
        rec_len = p->rec_len;

        if (rec_len < EXT2_DIR_REC_LEN(1))
            goto bad; /* too short */
        if (rec_len & 3)
            goto bad; /* misaligned */
        if (rec_len < EXT2_DIR_REC_LEN(p->name_len))
            goto bad; /* name doesn't fit */
        if (offs + rec_len > limit)
            goto bad; /* spans past the end */
        if (p->inode > fs->sb.inodes_count)
            goto bad; /* bogus inode number */
    }
    if (offs != limit)
        goto bad;
    return 0;

bad:
    pr_err("ext2: bad directory entry at offset %i", offs);
    return -1;
}

static inline int ext2_match(uint32_t len, const char *name, const ext2_dir_entry *de)
{
    if (len != de->name_len)
        return 0;
    if (!de->inode)
        return 0;
    return !memcmp(name, de->name, len);
}

static int ext2_find_entry(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, uint32_t *ino_out)
{
    uint32_t bs = fs->block_size;
    uint32_t nblocks = (dir->i_size + bs - 1) / bs; /* dir_pages() */
    int err = -ENOENT;

    if (nblocks == 0)
        return -ENOENT;

    uint8_t buf[bs];
    if (!buf)
        return -ENOMEM;

    for (uint32_t n = 0; n < nblocks; n++)
    {
        int64_t phys = ext2_bmap(fs, dir, n);
        if (phys <= 0)
        {
            err = -EIO;
            goto out;
        }
        if (ext2_read_block(fs, (uint32_t)phys, buf) != 0)
        {
            err = -EIO;
            goto out;
        }

        /* ext2_last_byte(): the final block may be only partly used */
        uint32_t last = dir->i_size - n * bs;
        if (last > bs)
            last = bs;

        if (ext2_check_dir_block(fs, buf, last) != 0)
        {
            err = -EIO;
            goto out;
        }

        ext2_dir_entry *de = (ext2_dir_entry *)buf;
        uint8_t *limit = buf + last - EXT2_DIR_REC_LEN(1);

        for (; (uint8_t *)de <= limit; de = (ext2_dir_entry *)((uint8_t *)de + de->rec_len))
        {
            if (de->rec_len == 0)
            {
                pr_err("ext2: zero-length directory entry\n");
                err = -EIO;
                goto out;
            }
            if (ext2_match(len, name, de))
            {
                *ino_out = de->inode;
                err = 0;
                goto out;
            }
        }
    }

out:
    return err;
}

int ext2_iget(ext2_t *fs, uint32_t ino, ext2_inode *out)
{
    uint32_t first_ino = (fs->sb.rev_level == 0) ? 11 : fs->sb.s_first_ino;

    /* reserved inodes (1, 3..10) are not valid targets, except root */
    if ((ino != EXT2_ROOT_INO && ino < first_ino) || ino > fs->sb.inodes_count)
        return -EINVAL;

    if (ext2_read_inode(fs, ino, out) != 0)
        return -EIO;

    /* deleted inode still referenced by a directory entry */
    if (out->i_links_count == 0 && (out->i_mode == 0 || out->i_dtime))
        return -ESTALE;

    return 0;
}

int ext2_lookup(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, ext2_inode *out, uint32_t *out_ino)
{
    uint32_t ino;
    int res;

    if ((dir->i_mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
        return -ENOTDIR;

    if (len > EXT2_NAME_LEN)
        return -ENAMETOOLONG;

    res = ext2_find_entry(fs, dir, name, len, &ino);

    if (res)
        return res;

    res = ext2_iget(fs, ino, out);
    if (res == -ESTALE)
    {
        pr_err("ext2_lookup: deleted inode referenced: %i\n", ino);
        return -EIO;
    }
    if (res)
        return res;

    if (out_ino)
        *out_ino = ino;
    return 0;
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

    uint32_t bgd_block = fs->sb.first_data_block + 1;
    uint32_t table_bytes = fs->group_count * sizeof(ext2_group_desc);
    uint32_t table_blocks = (table_bytes + fs->block_size - 1) / fs->block_size;

    static uint8_t blkbuf[4096];
    if (fs->block_size > sizeof(blkbuf))
        return -1;

    uint32_t copied = 0;
    for (uint32_t i = 0; i < table_blocks; i++)
    {
        if (ext2_read_block(fs, bgd_block + i, blkbuf) != 0)
            return -1;

        uint32_t chunk = table_bytes - copied;
        if (chunk > fs->block_size)
            chunk = fs->block_size;

        memcpy((uint8_t *)fs->bgd + copied, blkbuf, chunk);
        copied += chunk;
    }

    return 0;
}