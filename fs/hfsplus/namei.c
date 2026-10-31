#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#include "hfsplus.h"

static int find_cb(const hfsp_entry_t *e, void *ctx)
{
    find_ctx *f = ctx;
    if (e->parent != f->parent)
        return 0;

    const char *a = e->name, *b = f->name;
    for (; *a && *b; a++, b++)
        if (f->fold ? upper(*a) != upper(*b) : *a != *b)
            return 0;
    if (*a || *b)
        return 0;

    *f->out = *e;
    f->found = 1;
    return 1;
}

int hfsplus_lookup(hfsplus_t *fs, const char *path, hfsp_entry_t *out)
{
    uint32_t cur = HFSP_ROOT_CNID;
    char comp[256];

    while (*path == '/')
        path++;

    if (!*path)
    {
        memset(out, 0, sizeof *out);
        out->cnid = HFSP_ROOT_CNID;
        out->is_dir = 1;
        return 0;
    }

    for (;;)
    {
        uint32_t n = 0;

        while (*path && *path != '/')
        {
            if (n >= 255)
                return -4;
            comp[n++] = *path++;
        }

        comp[n] = '\0';
        while (*path == '/')
            path++;

        find_ctx fc = {cur, comp, !fs->is_hfsx, out, 0};
        int r = hfsp_scan(fs, find_cb, &fc);
        if (r < 0)
            return -1;

        if (!fc.found)
            return -2;

        if (!*path)
            return 0;

        if (!out->is_dir)
            return -3;

        cur = out->cnid;
    }
}