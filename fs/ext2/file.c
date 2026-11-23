#include <quernel/printk.h>
#include <drivers/rtc.h>
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

    f_inode->i_mode = S_IFREG | (mode & 0x0FFF);
    f_inode->i_size = 0;
    f_inode->i_blocks = 0;

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
    dir->i_atime = dir->i_ctime = unix_time_now(); 
    if (ext2_write_inode(fs, dir_ino, dir) != 0)
        r = -EIO;
out:
    kfree(buf);
    return r;
}

int ext2_write_file(ext2_t *fs, ext2_inode *inode, uint32_t ino, uint32_t offset, void *buf, int len)
{
    uint32_t bs = fs->block_size;
    uint32_t max_size = inode->i_size;

    if (len < 0)
        return -EINVAL;

    if (len == 0)
        return 0;

    if (S_ISDIR(inode->i_mode))
        return -EISDIR;
    if (!S_ISREG(inode->i_mode))
        return -EINVAL;

    uint64_t end = (uint64_t)offset + (uint64_t)len;
    if (end > 0xFFFFFFFFull)
        return -EFBIG;

    /* addressable blocks: 12 + P + P^2 + P^3 */
    uint64_t P = bs / 4;
    uint64_t max_blocks = 12 + P + P * P + P * P * P;
    // this was done in balloc as well for instance, because we cant do arithimathics with uint64_t numbers
    if ((((uint32_t)end) - 1) / ((uint32_t)bs) >= max_blocks)
        return -EFBIG;

    uint8_t *tmp = (uint8_t *)kmalloc(bs);
    if (!tmp)
        return -ENOMEM;

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t pos = offset;
    uint32_t remaining = (uint32_t)len;
    int written = 0;
    int err = 0;
    int dirty = 0;

    while (remaining > 0)
    {
        uint32_t lbn = pos / bs;
        uint32_t off = pos % bs;
        uint32_t chunk = bs - off;
        if (chunk > remaining)
            chunk = remaining;

        int64_t phys = ext2_bmap(fs, inode, lbn);
        if (phys < 0)
        {
            err = -EIO;
            break;
        }

        int is_new = 0;
        if (phys == 0)
        {
            uint32_t blk = ext2_alloc_block(fs, ino);
            if (blk == 0)
            {
                err = -ENOSPC;
                break;
            }

            int r = ext2_set_bmap(fs, inode, lbn, blk);

            if (r != 0)
            {
                ext2_free_block(fs, blk);
                err = r;
                break;
            }
            inode->i_blocks += bs / 512;
            phys = blk;
            is_new = 1;
            dirty = 1;
        }

        if (chunk == bs)
        {
            memcpy(tmp, (char *)src, bs);
        }
        else
        {
            if (is_new)
                memset(tmp, 0, bs);
            else if (ext2_read_block(fs, (uint32_t)phys, tmp) != 0)
            {
                err = -EIO;
                break;
            }
            memcpy(tmp + off, (char *)src, chunk);
        }

        if (ext2_write_block(fs, (uint32_t)phys, (uint16_t *)tmp) != 0)
        {
            err = -EIO;
            break;
        }

        src += chunk;
        pos += chunk;
        remaining -= chunk;
        written += (int)chunk;
    }

    kfree(tmp);

    if (written > 0 || dirty)
    {
        if (written > 0)
        {
            if (pos > inode->i_size)
                inode->i_size = pos;
            inode->i_mtime = inode->i_ctime = unix_time_now();
        }

        if (ext2_write_inode(fs, ino, inode) != 0)
            return -EIO;
    }

    return written > 0 ? written : err;
}

int64_t ext2_read_file(ext2_t *fs, const ext2_inode *ino, uint32_t offset, void *dst, uint32_t count)
{
    static uint8_t blkbuf[EXT2_MAX_BLOCK_SIZE];

    uint32_t bs = fs->block_size;
    uint32_t size = ino->i_size;
    uint8_t *out = (uint8_t *)dst;
    uint32_t done = 0;

    if (S_ISDIR(ino->i_mode))
        return -EISDIR;
    if (!S_ISREG(ino->i_mode))
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