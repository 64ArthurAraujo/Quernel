#pragma once
#include <stdint.h>
#include <drivers/ata.h>
#include <fs/hfs.h>

#define HFS_SIG     0x4244
#define HFSPLUS_SIG 0x482B
#define HFSX_SIG    0x4858

#define HFSP_MAX_NODE   4096
#define HFSP_ROOT_CNID  2

typedef struct {
    uint32_t       parent;
    const char    *name;
    int            fold;
    hfsp_entry_t  *out;
    int            found;
} find_ctx;


typedef int (*hfsp_cb)(const hfsp_entry_t *e, void *ctx);