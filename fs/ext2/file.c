#include <quernel/printk.h>
#include <fs/gpt.h>
#include <fs/ext.h>

#include "ext2.h"

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