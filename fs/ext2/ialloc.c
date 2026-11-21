#include <quernel/printk.h>
#include <stdint.h>
#include <string.h>

#include "ext2.h"

ext2_inode *ext2_new_inode(ext2_t *fs, ext2_inode *dir, uint32_t dir_ino, uint16_t mode)
{
    if (fs->sb.free_inodes_count == 0) 
    {
        pr_info("There are no more free inodes in the superblock!\n");
        return NULL;
    }

    uint32_t group = (dir_ino - 1) / fs->sb.inodes_per_group;
    uint32_t total_groups = (fs->sb.blocks_count + fs->sb.blocks_per_group - 1) / fs->sb.blocks_per_group;

    int f_group = -1;
    for (uint32_t i = 0; i < total_groups; i++)
    {
        uint32_t curr = (group + i) % total_groups;
        if (fs->bgd[curr].free_inodes_count > 0)
        {
            f_group = curr;
            break;
        }
    }

    if (f_group == -1)
    {
        pr_info("Failed to create new inode, unable to found a group with free inodes!\n");
        return NULL;
    }

    uint8_t *bitmap = kmalloc(fs->block_size);

    if (!bitmap)
        return NULL;
    
    if (ext2_read_block(fs, fs->bgd[f_group].inode_bitmap, bitmap) != 0)
    {
        kfree(bitmap);
        return NULL;
    }

    uint32_t start_bit = (f_group == 0) ? (fs->sb.s_first_ino - 1) : 0;

    int f_bit = -1;

    for (uint32_t bit = start_bit; bit < fs->sb.inodes_per_group; bit++)
    {
        if (!(bitmap[bit / 8] & (1 << (bit % 8))))
        {
            bitmap[bit / 8] |= (1 << (bit % 8));
            f_bit = bit;
            break;
        }
    }

    if (f_bit == -1)
    {
        kfree(bitmap);
        pr_info("Failed to create new inode, unable to find a free bit!\n");
        return NULL;
    }

    ext2_write_block(fs, fs->bgd[f_group].inode_bitmap, (uint16_t *) bitmap);
    kfree(bitmap);

    fs->bgd[f_group].free_inodes_count--;
    if (S_ISDIR(mode))
    {
        fs->bgd[f_group].used_dirs_count++;
    }

    ext2_write_bgdt(fs, f_group);

    fs->sb.free_inodes_count--;
    ext2_write_super_block(fs);

    uint32_t new_ino = (f_group * fs->sb.inodes_per_group) + f_bit + 1;

    ext2_inode *inode = kmalloc(sizeof(ext2_inode));

    if (!inode)
    {
        ext2_free_inode(fs, new_ino, S_ISDIR(mode));
        return NULL;
    }
    memset(inode, 0, sizeof(ext2_inode));

    inode->i_mode = mode;
    inode->i_links_count = 1;
    // This is most probably not ideal but this is the only way i see passing the ino alongside the inode struct. 
    inode->osd1.masix1.m_i_reserved1 = new_ino;
    // Since we don't have a way to get the current date, set to 0 for now which is the unix epoch "Thu Jan 01 00:00:00 1970 UTC"
    inode->i_atime = inode->i_ctime = inode->i_mtime = 67; 
    
    return inode;
}

int ext2_free_inode(ext2_t *fs, uint32_t ino, int isdir)
{
    if (ino == 0 || ino > fs->sb.inodes_count)
        return -EINVAL;

    uint32_t group = (ino - 1) / fs->sb.inodes_per_group;
    uint32_t bit = (ino - 1) % fs->sb.inodes_per_group;

    uint8_t *bitmap = (uint8_t *)kmalloc(fs->block_size);
    if (!bitmap)
        return -ENOMEM;

    int r = 0;

    if (ext2_read_block(fs, fs->bgd[group].inode_bitmap, bitmap) != 0)
    {
        r = -EIO;
        goto out;
    }

    if (!(bitmap[bit / 8] & (1 << (bit % 8))))
    {
        r = -EINVAL;
        goto out;
    }

    bitmap[bit / 8] &= ~(1 << (bit % 8));

    if (ext2_write_block(fs, fs->bgd[group].inode_bitmap, (uint16_t *)bitmap) != 0)
    {
        r = -EIO;
        goto out;
    }

    fs->bgd[group].free_blocks_count++;
    if (isdir && fs->bgd[group].used_dirs_count > 0)
        fs->bgd[group].used_dirs_count--;
    fs->sb.free_inodes_count++;

    if (ext2_write_bgdt(fs, group) != 0 || ext2_write_super_block(fs) != 0)
        r = -EIO;

out:
    kfree(bitmap);
    return r;
}