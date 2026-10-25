#include <quernel/printk.h>
#include <memory.h>
#include <stddef.h>

#include "ext2.h"

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

        uint32_t len = 0;
        while (path[len] && path[len] != '/')
            len++;
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