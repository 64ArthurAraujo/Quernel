#include <quernel/printk.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#include "hfsplus.h"

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