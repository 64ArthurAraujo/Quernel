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

/* super.c */
void parse_fork(const uint8_t *p, hfsp_fork_t *f);
int hfsp_read_bytes(hfsplus_t *fs, const hfsp_fork_t *f, uint32_t off, void *dst, uint32_t len);
int hfsplus_mount(hfsplus_t *fs, const partition_t *part);

/* dir.c */
int hfsp_parse_record(const uint8_t *rec, uint32_t avail, hfsp_entry_t *e);
int hfsp_scan(hfsplus_t *fs, hfsp_cb cb, void *ctx);
void hfsplus_ls(hfsplus_t *fs, uint32_t dir_cnid);

/* namei.c */
int hfsplus_lookup(hfsplus_t *fs, const char *path, hfsp_entry_t *out);

/* file.c */
int32_t hfsplus_read(hfsplus_t *fs, const hfsp_entry_t *e, uint32_t offset, void *dst, uint32_t max);