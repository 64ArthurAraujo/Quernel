#include <quernel/printk.h>
#include <drivers/ata.h>
#include <string.h>

#include "fat.h"

uint32_t cluster_to_sector(const fat12_t *fs, uint16_t c)
{
    return fs->data_start + (uint32_t)(c - 2) * fs->sectors_per_cluster;
}

uint16_t fat12_next(const fat12_t *fs, uint16_t c)
{
    uint32_t off = c + (c / 2);
    uint16_t v = fs->fat[off] | (fs->fat[off + 1] << 8);
    return (c & 1) ? (v >> 4) : (v & 0x0FFF);
}

static int scan_sector(const uint8_t *sec, fat12_dir_cb cb, void *ctx)
{
    for (int i = 0; i < 16; i++)
    {
        const struct fat_dirent *e = (const struct fat_dirent *)(sec + i * 32);
        uint8_t first = (uint8_t)e->name[0];

        if (first == 0x00)
            return 2; /* no more entries */
        if (first == 0xE5)
            continue; /* deleted */
        if (e->attr == ATTR_LFN)
            continue; /* long-name fragment */
        if (e->attr & ATTR_VOLUME)
            continue; /* volume label */
        if (cb(e, ctx))
            return 1;
    }
    return 0;
}

int fat12_walk_dir(fat12_t *fs, uint16_t dir_cluster, fat12_dir_cb cb, void *ctx)
{
    uint8_t sec[512] __aligned(2);
    int r;

    if (dir_cluster == 0)
    {
        for (uint32_t s = 0; s < fs->root_sectors; s++)
        {
            if (read_partition(fs->part, fs->root_start + s, sec) != 0)
                return -1;
            r = scan_sector(sec, cb, ctx);

            if (r == 1)
                return 1;
            if (r == 2)
                return 0;
        }

        return 0;
    }

    uint16_t c = dir_cluster;
    uint32_t guard = fs->cluster_count + 2;

    while (FAT12_VALID(c) && guard--)
    {
        uint32_t base = cluster_to_sector(fs, c);

        for (uint32_t s = 0; s < fs->sectors_per_cluster; s++)
        {
            if (read_partition(fs->part, base + s, sec) != 0)
                return -1;
            r = scan_sector(sec, cb, ctx);

            if (r == 1)
                return 1;
            if (r == 2)
                return 0;
        }

        c = fat12_next(fs, c);
    }

    return 0;
}

static int print_entry(const struct fat_dirent *e, void *ctx)
{
    (void)ctx;
    char name[13], num[11];

    fat12_format_name(e, name);
    pr_debug("%s", name);

    if (e->attr & ATTR_DIR)
    {
        pr_debug("  <DIR>\n");
    }
    else
    {
        u32_to_str(e->size, num);
        pr_debug("  %s bytes\n", num);
    }
    return 0;
}

void fat12_ls(fat12_t *fs, uint16_t dir_cluster)
{
    fat12_walk_dir(fs, dir_cluster, print_entry, 0);
}