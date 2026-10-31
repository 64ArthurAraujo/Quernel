#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#include "hfsplus.h"

void parse_fork(const uint8_t *p, hfsp_fork_t *f)
{
    f->size = be64(p);
    f->total_blocks = be32(p + 12);

    for (int i = 0; i < 8; i++)
    {
        f->ext[i].start = be32(p + 16 + i * 8);
        f->ext[i].count = be32(p + 20 + i * 8);
    }
}

int hfsp_read_bytes(hfsplus_t *fs, const hfsp_fork_t *f, uint32_t off, void *dst, uint32_t len)
{
    uint8_t sec[512] __aligned(2);
    uint8_t *out = dst;

    while (len > 0)
    {
        uint32_t base = 0;
        int i = 0;
        int found = 0;

        for (i = 0; i < 8 && f->ext[i].count != 0; i++)
        {
            uint32_t ebytes = f->ext[i].count * fs->block_size;
            if (off < base + ebytes)
            {
                found = 1;
                break;
            }
            base += ebytes;
        }

        if (!found)
            return -2;

        uint32_t int_ext = off - base;
        uint32_t avail = f->ext[i].count * fs->block_size - int_ext;
        uint32_t take = len < avail ? len : avail;
        uint32_t vol = f->ext[i].start * fs->block_size + int_ext;

        while (take > 0)
        {
            uint32_t s = vol >> 9;
            uint32_t o = vol & 511;
            uint32_t c = 512 - o;

            if (c > take)
                c = take;

            if (read_partition(&fs->part, s, sec) != 0)
                return -1;

            memcpy(out, sec + o, c);
            out += c;
            vol += c;
            off += c;
            len -= c;
            take -= c;
        }
    }

    return 0;
}

/* returns 0 ok; -1 read error, -2 no HFS+ signature, -3 bad header values, -6 classic HFS */
int hfsplus_mount(hfsplus_t *fs, const partition_t *part)
{
    uint8_t sec[512] __aligned(2);
    hfsplus_t t;

    if (!fs || !part)
        return -1;

    t.part = *part;

    if (read_partition(&t.part, 2, sec) != 0)
        return -1;

    uint16_t sig = be16(sec);
    if (sig == HFS_SIG)
        return -6;
    if (sig != HFSPLUS_SIG && sig != HFSX_SIG)
        return -2;

    t.is_hfsx = (sig == HFSX_SIG);

    t.block_size = be32(sec + 40);
    t.total_blocks = be32(sec + 44);

    if (t.block_size < 512 || (t.block_size & (t.block_size - 1)))
        return -3;

    if ((uint64_t)t.block_size * t.total_blocks > (uint64_t)part->count * 512)
        return -3;

    t.sectors_per_block = t.block_size / 512;

    parse_fork(sec + 112, &t.allocation);
    parse_fork(sec + 192, &t.extents);
    parse_fork(sec + 272, &t.catalog);

    if (t.catalog.size == 0 || t.catalog.ext[0].count == 0)
        return -3;

    uint8_t hdr[48];
    if (hfsp_read_bytes(&t, &t.catalog, 0, hdr, sizeof hdr) != 0)
        return -1;

    t.first_leaf = be32(hdr + 24);
    t.node_size = be16(hdr + 32);

    if (t.node_size < 512 || t.node_size > HFSP_MAX_NODE || (t.node_size & (t.node_size - 1)))
        return -3;

    if ((uint64_t)t.block_size * t.total_blocks > 0xFFFFFFFFu)
        return -3;

    *fs = t;
    return 0;
}
