#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>

#include "hfsplus.h"
#include "utils.h"

static void parse_fork(const uint8_t *p, hfsp_fork_t *f)
{
    f->size = be64(p);
    f->total_blocks = be32(p + 12);

    for (int i = 0; i < 8; i++)
    {
        f->ext[i].start = be32(p + 16 + i * 8);
        f->ext[i].count = be32(p + 20 + i * 8);
    }
}

static void print_kv(const char *label, uint32_t v)
{
    // char num[12];
    // u32_to_str(v, num);
    // printk("%s%s\n", label, num);
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

    if ((uint64_t) t.block_size * t.total_blocks > (uint64_t)part->count * 512)
        return -3;

    t.sectors_per_block = t.block_size / 512;

    parse_fork(sec + 112, &t.allocation);
    parse_fork(sec + 192, &t.extents);
    parse_fork(sec + 272, &t.catalog);

    if (t.catalog.size == 0 || t.catalog.ext[0].count == 0)
        return -3;
    
    print_kv("hfs+ block size: ", t.block_size);
    print_kv("hfs+ total blocks: ", t.total_blocks);
    print_kv("catalog first block: ", t.catalog.ext[0].start);
    print_kv("catalog blocks: ", t.catalog.ext[0].count);

    *fs = t;
    return 0;
}