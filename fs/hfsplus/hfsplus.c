#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

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

static int hfsp_parse_record(const uint8_t *rec, uint32_t avail, hfsp_entry_t *e)
{
    if (avail < 8)
        return 0;

    uint32_t key_len = be16(rec);
    uint32_t name_len = be16(rec + 6);

    if (name_len > 255 || 8 + name_len * 2 > avail)
        return 0;

    uint32_t d = 2 + key_len;
    if (d + 12 > avail)
        return 0;

    uint16_t type = be16(rec + d);
    if (type == 1)
        e->is_dir = 1;
    else if (type == 2)
        e->is_dir = 0;
    else
        return 0;

    e->parent = be32(rec + 2);
    e->cnid = be32(rec + d + 8);

    for (uint32_t i = 0; i < name_len; i++)
    {
        uint16_t c = be16(rec + 8 + i * 2);
        e->name[i] = c < 128 ? (char)c : '?';
    }

    e->name[name_len] = '\0';
    e->size = 0;

    memset(&e->fork, 0, sizeof e->fork);

    if (!e->is_dir)
    {
        if (d + 168 > avail)
            return 0;

        parse_fork(rec + d + 88, &e->fork);
        e->size = e->fork.size >> 32 ? 0xFFFFFFFFu : (uint32_t)e->fork.size;
    }

    return 1;
}

static int hfsp_scan(hfsplus_t *fs, hfsp_cb cb, void *ctx)
{
    static uint8_t node[HFSP_MAX_NODE] __aligned(2);
    hfsp_entry_t e;

    uint32_t n = fs->first_leaf;
    uint32_t guard = (uint32_t)fs->catalog.size / fs->node_size + 1;

    while (n != 0 && guard--)
    {
        if (hfsp_read_bytes(fs, &fs->catalog, n * fs->node_size, node, fs->node_size) != 0)
            return -1;

        if (node[8] != 0xFF)
            break;

        uint32_t next = be32(node);
        uint32_t nrec = be16(node + 10);
        if (2 * (nrec + 1) > fs->node_size)
            break;

        for (uint32_t i = 0; i < nrec; i++)
        {
            uint32_t slot = fs->node_size - 2 * (i + 1);
            uint32_t rec = be16(node + slot);

            if (rec < 14 || rec + 8 > fs->node_size)
                continue;

            if (hfsp_parse_record(node + rec, fs->node_size - rec, &e))
            {
                if (cb(&e, ctx))
                    return 1;
            }
        }
        n = next;
    }
    return 0;
}

static int ls_cb(const hfsp_entry_t *e, void *ctx)
{
    if (e->parent != *(uint32_t *)ctx)
        return 0;

    char num[12];
    u32_to_str(e->size, num);

    if (e->is_dir)
        printk("%s  <DIR>\n", e->name);
    else
        printk("%s  %s bytes\n", e->name, num);

    return 0;
}

static int dump_cb(const hfsp_entry_t *e, void *ctx)
{
    (void)ctx;
    char p[12], c[12];
    u32_to_str(e->parent, p);
    u32_to_str(e->cnid, c);
    printk("parent=%s cnid=%s %s '%s'\n",
           p, c, e->is_dir ? "dir " : "file", e->name);
    return 0;
}

void hfsplus_ls(hfsplus_t *fs, uint32_t dir_cnid)
{
    hfsp_scan(fs, ls_cb, &dir_cnid);
}

static char fold_c(char c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static int find_cb(const hfsp_entry_t *e, void *ctx)
{
    find_ctx *f = ctx;
    if (e->parent != f->parent)
        return 0;

    const char *a = e->name, *b = f->name;
    for (; *a && *b; a++, b++)
        if (f->fold ? fold_c(*a) != fold_c(*b) : *a != *b)
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

int32_t hfsplus_read(hfsplus_t *fs, const hfsp_entry_t *e, uint32_t offset, void *dst, uint32_t max)
{
    if (e->is_dir)
        return -3;

    if (offset >= e->size)
        return 0;

    uint32_t n = e->size - offset;

    if (n > max)
        n = max;

    if (n == 0)
        return 0;

    int r = hfsp_read_bytes(fs, &e->fork, offset, dst, n);
    
    if (r != 0)
        return r;

    return (int32_t)n;
}