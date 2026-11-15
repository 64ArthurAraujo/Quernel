#include <quernel/printk.h>
#include <drivers/ata.h>
#include <memory.h>

#include "ext2.h"

int ext2_check_dir_block(ext2_t *fs, const uint8_t *buf, uint32_t limit)
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

int ext2_match(uint32_t len, const char *name, const ext2_dir_entry *de)
{
    if (len != de->name_len)
        return 0;
    if (!de->inode)
        return 0;
    return !memcmp(name, de->name, len);
}

int ext2_find_entry(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, uint32_t *ino_out)
{
    uint32_t bs = fs->block_size;
    uint32_t nblocks = (dir->i_size + bs - 1) / bs; /* dir_pages() */
    int err = -ENOENT;

    if (nblocks == 0)
        return -ENOENT;

    uint8_t *buf = (uint8_t *)kmalloc(bs);
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
    kfree(buf);
    return err;
}

int dir_block_has_name(uint8_t *buf, uint32_t limit, const char *name, uint8_t name_len)
{
    uint32_t off = 0;

    while (off + 8 <= limit)
    {
        ext2_dir_entry *de = (ext2_dir_entry *)(buf + off);

        if (de->rec_len < 8)
            break;

        if (de->inode && de->name_len == name_len && memcmp(de->name, name, name_len) == 0)
            return 1;

        off += de->rec_len;
    }

    return 0;
}

int dir_block_insert(uint8_t *buf, uint32_t limit, uint32_t ino, const char *name, uint8_t name_len, uint8_t f_type)
{
    uint32_t need = EXT2_DIR_REC_LEN(name_len);
    uint32_t off = 0;

    while (off + 8 <= limit)
    {
        ext2_dir_entry *de = (ext2_dir_entry *)(buf + off);

        if (de->rec_len < 8)
            return 0;

        uint32_t real = de->inode ? EXT2_DIR_REC_LEN(de->name_len) : 0;
        uint32_t slack = de->rec_len - real;

        if (slack >= need)
        {
            ext2_dir_entry *n_de;

            if (real == 0)
            {
                n_de = de;
            }
            else
            {
                n_de = (ext2_dir_entry *)(buf + off + real);
                n_de->rec_len = de->rec_len - real;
                de->rec_len = real;
            }

            n_de->inode = ino;
            n_de->name_len = name_len;
            n_de->file_type = f_type;

            memcpy(n_de->name, (char *)name, name_len);
            return 1;
        }

        off += de->rec_len;
    }

    return 0;
}