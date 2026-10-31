#include <quernel/printk.h>
#include <string.h>

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
int ext2_block_to_path(ext2_t *fs, uint32_t i_block, uint32_t offsets[4])
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
