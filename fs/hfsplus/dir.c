#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#include "hfsplus.h"

int hfsp_parse_record(const uint8_t *rec, uint32_t avail, hfsp_entry_t *e)
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

int hfsp_scan(hfsplus_t *fs, hfsp_cb cb, void *ctx)
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
        pr_debug("%s  <DIR>\n", e->name);
    else
        pr_debug("%s  %s bytes\n", e->name, num);

    return 0;
}

void hfsplus_ls(hfsplus_t *fs, uint32_t dir_cnid)
{
    hfsp_scan(fs, ls_cb, &dir_cnid);
}
