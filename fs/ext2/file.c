#include <quernel/printk.h>
#include <fs/gpt.h>
#include <fs/ext.h>

#include <string.h>

#include "ext2.h"

int ext2_create_file(ext2_t *fs, ext2_inode *dir, uint32_t dir_ino, char *name, uint16_t mode)
{
    int r = 0;
    uint32_t bs = fs->block_size;
    size_t len = strlen(name);

    if (len == 0 || len > EXT2_NAME_LEN)
        return -ENAMETOOLONG;

    uint8_t name_len = (uint8_t)len;

    uint32_t n_blks = (dir->i_size + bs - 1) / bs;

    if (n_blks == 0)
        return -EIO;

    uint8_t *buf = (uint8_t *)kmalloc(bs);

    if (!buf)
        return -ENOMEM;

    for (uint32_t n = 0; n < n_blks; n++)
    {
        int64_t phys = ext2_bmap(fs, dir, n);

        if (phys <= 0 || ext2_read_block(fs, (uint32_t)phys, buf) != 0)
        {
            r = -EIO;
            goto out;
        }

        uint32_t last = dir->i_size - n * bs;
        if (last > bs)
            last = bs;

        if (ext2_check_dir_block(fs, buf, last) != 0)
        {
            r = -EIO;
            goto out;
        }

        if (dir_block_has_name(buf, last, name, name_len))
        {
            r = -EEXIST;
            goto out;
        }
    }

    ext2_inode *f_inode = ext2_new_inode(fs, dir, dir_ino, mode);

    // Here osd1.masix1.m_i_reserved1 contains the ino
    uint32_t f_ino = f_inode->osd1.masix1.m_i_reserved1;

    if (f_ino == 0)
    {
        r = -ENOSPC;
        goto out;
    }

    f_inode->i_mode = EXT2_S_IFREG | (mode & 0x0FFF);
    f_inode->i_size = 0;
    f_inode->i_blocks = 0;

    // When creating ext2_new_inode ive set it to automatically write the inode to the disk
    // but here we would want to change other properties of the inode before writing it.
    // Remember later to remove the ext2_write_inode from ext2_new_inode so that we dont need to
    // write it two times for no reason like we are going to do right bellow.
    if (ext2_write_inode(fs, f_ino, f_inode) != 0)
    {
        r = -EIO;
        goto out;
    }

    for (uint32_t n = 0; n < n_blks; n++)
    {
        int64_t phys = ext2_bmap(fs, dir, n);
        if (phys <= 0 || ext2_read_block(fs, (uint32_t)phys, buf) != 0)
        {
            r = -EIO;
            goto out;
        }

        uint32_t last = dir->i_size - n * bs;
        if (last > bs)
            last = bs;

        if (dir_block_insert(buf, last, f_ino, name, name_len, 1 /* file (probably) */))
        {
            if (ext2_write_block(fs, (uint32_t)phys, (uint16_t *) buf) != 0)
            {
                r = -EIO;
                goto out;
            }
            goto update_dir;
        }
    }

update_dir:
    dir->i_atime = dir->i_ctime = 67; // again we will need to deal with that later when we can get the unix timestamp
    if (ext2_write_inode(fs, dir_ino, dir) != 0)
        r = -EIO;
out:
    kfree(buf);
    return r;
}

int64_t ext2_read_file(ext2_t *fs, const ext2_inode *ino, uint32_t offset, void *dst, uint32_t count)
{
    static uint8_t blkbuf[EXT2_MAX_BLOCK_SIZE];

    uint32_t bs = fs->block_size;
    uint32_t size = ino->i_size;
    uint8_t *out = (uint8_t *)dst;
    uint32_t done = 0;

    if ((ino->i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR)
        return -EISDIR;
    if ((ino->i_mode & EXT2_S_IFMT) != EXT2_S_IFREG)
        return -EINVAL;

    if (offset > size)
        return 0;

    if (count >= size - offset)
        count = size - offset;

    while (done < count)
    {
        uint32_t pos = offset + done;
        uint32_t i_block = pos / bs;
        uint32_t off_in_blk = pos % bs;
        uint32_t chunk = bs - off_in_blk;
        if (chunk > count - done)
            chunk = count - done;

        int64_t phys = ext2_bmap(fs, ino, i_block);
        if (phys < 0)
            return -EIO;

        if (phys == 0)
        {
            memset(out + done, 0, chunk);
        }
        else if (chunk == bs)
        {
            if (ext2_read_block(fs, (uint32_t)phys, out + done) != 0)
                return -EIO;
        }
        else
        {
            if (ext2_read_block(fs, (uint32_t)phys, blkbuf) != 0)
                return -EIO;
            memcpy(out + done, blkbuf + off_in_blk, chunk);
        }

        done += chunk;
    }
    return done;
}