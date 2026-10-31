#include <quernel/printk.h>
#include <drivers/ata.h>
#include <string.h>

#include "fat.h"

void fat12_format_name(const struct fat_dirent *e, char *out)
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

int fat12_make_83(const char *s, char out[11])
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