#include "../../drivers/ata.h"
#include "../../libc/string.h"
#include <quernel/printk.h>
#include "fat.h"

/**
 * Performs checks and returns 1 if valid, 0 on error.
 */
int is_bootsect_valid(const struct fat12_bpb *b_sec)
{
    if (b_sec->bytes_per_sector != 512)
    {
        printk("Invalid size of bytes per sector, not 512!\n");
        return 0;
    }

    if (b_sec->sectors_per_cluster == 0 || (b_sec->sectors_per_cluster & (b_sec->sectors_per_cluster - 1)))
    {
        printk("Invalid number of sectors per cluster!\n");
        return 0;
    }

    return 1;
}

/**
 * Attempts to mount a fat12 file system from the partition
 * and returns the following status codes:
 * `-1` read error, `-2` no boot signature, `-3` bad or unsupported BPB, `-4` FAT too large for the buffer, `-5` not FAT12.
 */
int fat12_mount(fat12_t *fs, partition_t *part)
{
    uint8_t sec[512] __aligned(2);

    if (read_partition(part, 0, sec) != 0)
    {
        printk("Failed to read partition!\n");
        return -1;
    }

    if (sec[510] != 0x55 || sec[511] != 0xAA)
        return -2;

    const struct fat12_bpb *b = (const struct fat12_bpb *)sec;

    if (is_bootsect_valid(b) == 0)
    {
        printk("Failed to validate the boot sector!\n");
        return -3;
    }

    uint32_t total = b->total_sectors_16 ? b->total_sectors_16 : b->total_sectors_32;

    fs->part = part;
    fs->sectors_per_cluster = b->sectors_per_cluster;
    fs->fat_start = b->reserved_sectors;
    fs->root_start = fs->fat_start + b->num_fats * b->sectors_per_fat;
    fs->root_sectors = (b->root_entry_count * 32 + 511) / 512;
    fs->data_start = fs->root_start + fs->root_sectors;

    if (total <= fs->data_start)
    {
        printk("Total of sectors is less or equal to the data_start!\n");
        return -3;
    }

    fs->cluster_count = (total - fs->data_start) / fs->sectors_per_cluster;

    if (fs->cluster_count >= 4085)
    {
        printk("Cluster count exceeds 4085, this partition is likely not a FAT12 partition!\n");
        return -5;
    }

    for (uint32_t i = 0; i < b->sectors_per_fat; i++)
    {
        // Load the data into fs->fat
        if (read_partition(part, fs->fat_start + i, fs->fat + i * 512) != 0)
        {
            printk("Failed to read filesys data!\n");
            return -1;
        }
    }

    return 0;
}

static uint16_t fat12_next(const fat12_t *fs, uint16_t c)
{
    uint32_t off = c + (c / 2);
    uint16_t v = fs->fat[off] | (fs->fat[off + 1] << 8);
    return (c & 1) ? (v >> 4) : (v & 0x0FFF);
}

static uint32_t cluster_to_sector(const fat12_t *fs, uint16_t c)
{
    return fs->data_start + (uint32_t)(c - 2) * fs->sectors_per_cluster;
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

static char upper(char c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static void fat12_format_name(const struct fat_dirent *e, char *out)
{
    int p = 0;

    for (int i = 0; i < 8 && e->name[i] != ' '; i++)
        out[p++] = e->name[i];

    if (e->name[8] != ' ')
    {
        out[p++] = '.';
        for (int i = 8; i < 11 && e->name[i] != ' '; i++)
            out[p++] = e->name[i];
    }

    out[p] = '\0';
}

static int fat12_make_83(const char *s, char out[11])
{
    int n = 0;
    for (int i = 0; i < 11; i++)
        out[i] = ' ';

    for (; *s && *s != '.'; s++)
    {
        if (n >= 8)
            return -1;
        out[n++] = upper(*s);
    }

    if (*s == '.')
    {
        s++;
        n = 8;

        for (; *s; s++)
        {
            if (n >= 11)
                return -1;
            out[n++] = upper(*s);
        }
    }

    return 0;
}

static int print_entry(const struct fat_dirent *e, void *ctx)
{
    (void)ctx;
    char name[13], num[11];

    fat12_format_name(e, name);
    printk("%s", name);

    if (e->attr & ATTR_DIR)
    {
        printk("  <DIR>\n");
    }
    else
    {
        u32_to_str(e->size, num);
        printk("  %s bytes\n", num);
    }
    return 0;
}

void fat12_ls(fat12_t *fs, uint16_t dir_cluster)
{
    fat12_walk_dir(fs, dir_cluster, print_entry, 0);
}

static int find_cb(const struct fat_dirent *e, void *ctx)
{
    struct find_ctx *f = ctx;

    for (int i = 0; i < 11; i++)
        if (e->name[i] != f->name[i])
            return 0;

    f->found = *e;
    f->ok = 1;

    return 1;
}

int fat12_lookup(fat12_t *fs, const char *path, struct fat_dirent *out)
{
    uint16_t dir = 0;
    struct find_ctx fc;

    while (*path == '/')
        path++;

    if (!*path)
        return -1;

    for (;;)
    {
        char comp[16];
        int n = 0;

        while (*path && *path != '/')
        {
            if (n >= 15)
                return -1;
            comp[n++] = *path++;
        }
        comp[n] = '\0';
        while (*path == '/')
            path++;

        if (fat12_make_83(comp, fc.name) != 0)
            return -1;

        fc.ok = 0;

        if (fat12_walk_dir(fs, dir, find_cb, &fc) < 0 || !fc.ok)
            return -1;

        if (!*path)
        {
            /* last component */
            *out = fc.found;
            return 0;
        }

        /* not a directory */
        if (!(fc.found.attr & ATTR_DIR))
            return -1;

        dir = fc.found.cluster_lo;
    }
}

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