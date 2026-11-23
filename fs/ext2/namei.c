#include <quernel/printk.h>
#include <drivers/rtc.h>
#include <memory.h>
#include <stddef.h>
#include <string.h>

#include "ext2.h"

/* Calculates the length of the specified path */
uint32_t pathlen(const char *path)
{
    uint32_t len = 0;
    while (path[len] && path[len] != '/')
        len++;
    return len;
}

/* Resolves an absolute path. */
int ext2_namei(ext2_t *fs, const char *path, ext2_inode *out, uint32_t *out_ino)
{
    ext2_inode cur;
    ext2_inode next;
    uint32_t cur_ino = EXT2_ROOT_INO;
    uint32_t next_ino;
    int res;

    if (!path || path[0] != '/')
        return -EINVAL;

    res = ext2_iget(fs, EXT2_ROOT_INO, &cur);
    if (res)
        return res;

    while (*path)
    {
        while (*path == '/')
            path++;
        if (!*path)
            break;

        uint32_t len = pathlen(path);

        if (len > EXT2_NAME_LEN)
            return -ENAMETOOLONG;

        /* ext2_lookup checks that `cur` is a directory (-ENOTDIR) */
        res = ext2_lookup(fs, &cur, path, len, &next, &next_ino);
        if (res)
            return res; /* includes -ENOENT */

        cur = next;
        cur_ino = next_ino;
        path += len;
    }

    if (out)
        *out = cur;
    if (out_ino)
        *out_ino = cur_ino;
    return 0;
}

int ext2_lookup(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, ext2_inode *out, uint32_t *out_ino)
{
    uint32_t ino;
    int res;

    if (!S_ISDIR(dir->i_mode))
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

int ext2_mkdir(ext2_t *fs, ext2_inode *dir, uint32_t dir_ino, const char *name, int len, int mode)
{
    if (len == 0 || len > EXT2_NAME_LEN)
        return -ENAMETOOLONG;
    if (!S_ISDIR(dir->i_mode))
        return -ENOTDIR;
    if (dir->i_links_count >= 32000)
        return -ENAMETOOLONG; // this is other error but i will change later

    if (ext2_find_entry(fs, dir, name, len, NULL) == 0)
        return -EEXIST;

    ext2_inode *inode;
    inode = ext2_new_inode(fs, dir, dir_ino, S_IFDIR);
    uint32_t inode_ino = 0;
    int r = 0;

    if (!inode)
        return -ENOSPC;

    inode_ino = inode->osd1.masix1.m_i_reserved1;

    uint8_t *buf = kmalloc(fs->block_size);

    if (!buf)
    {
        r = -ENOMEM;
        goto err_free_inode;
    }

    uint32_t blk = ext2_alloc_block(fs, inode_ino); // later i need specify each type of exception inside of ext2_alloc_block

    if (blk == 0)
    {
        r = -ENOSPC;
        goto err_free_buf;
    }

    r = ext2_set_bmap(fs, inode, 0, blk);
    if (r != 0)
        goto err_free_blk;

    memset(buf, 0, fs->block_size);

    // The dot folder
    ext2_dir_entry *de = (ext2_dir_entry *)buf;
    de->inode = inode_ino;
    de->file_type = 2; 
    de->rec_len = EXT2_DIR_REC_LEN(1);
    de->name_len = 1;
    de->name[0] = '.';

    // the dot-dot folder
    de = (ext2_dir_entry *)((char *)buf + EXT2_DIR_REC_LEN(1));
    de->inode = dir_ino;
    de->file_type = 2;
    de->rec_len = fs->block_size - EXT2_DIR_REC_LEN(1);
    de->name_len = 2;
    de->name[0] = '.';
    de->name[1] = '.';

    if (ext2_write_block(fs, blk, (uint16_t *)buf) != 0)
    {
        r = -EIO;
        goto err_free_blk;
    }

    inode->i_size = fs->block_size;
    inode->i_blocks = fs->block_size / 512;
    inode->i_links_count = 2;
    inode->i_mode = S_IFDIR | (mode & 0777);
    if (dir->i_mode & S_ISGID)
        inode->i_mode |= S_ISGID;
    inode->i_mtime = inode->i_atime = unix_time_now();

    if (ext2_write_inode(fs, inode_ino, inode) < 0)
    {
        r = -EIO;
        goto err_free_blk;
    }

    r = ext2_add_link(fs, dir, dir_ino, name, (uint8_t)len, inode_ino, 2);
    if (r != 0)
        goto err_free_blk;

    dir->i_links_count++;
    dir->i_mtime = dir->i_ctime = unix_time_now();

    if (ext2_write_inode(fs, dir_ino, dir) != 0)
        r = -EIO;
    else
        r = 0;

    kfree(buf);
    kfree(inode);
    return r;

err_free_blk:
    ext2_free_block(fs, blk);
err_free_buf:
    kfree(buf);
err_free_inode:
    ext2_free_inode(fs, inode_ino, 1);
    kfree(inode);
    return r;
}