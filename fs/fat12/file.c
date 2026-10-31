#include <quernel/printk.h>
#include <drivers/ata.h>
#include <string.h>

#include "fat.h"

int32_t fat12_read(fat12_t *fs, const struct fat_dirent *e, void *dst, uint32_t max)
{
    uint8_t sec[512] __aligned(2);
    uint8_t *out = dst;
    uint32_t remaining = e->size < max ? e->size : max;
    uint32_t total = 0;
    uint16_t c = e->cluster_lo;
    uint32_t guard = fs->cluster_count + 2;

    while (remaining > 0 && FAT12_VALID(c) && guard--)
    {
        uint32_t base = cluster_to_sector(fs, c);
        for (uint32_t s = 0; s < fs->sectors_per_cluster && remaining > 0; s++)
        {
            if (read_partition(fs->part, base + s, sec) != 0)
                return -1;

            uint32_t n = remaining < 512 ? remaining : 512;

            for (uint32_t i = 0; i < n; i++)
                out[total + i] = sec[i];

            total += n;
            remaining -= n;
        }
        c = fat12_next(fs, c);
    }
    return (int32_t)total;
}