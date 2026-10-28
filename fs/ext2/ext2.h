#pragma once
#include <stdint.h>
#include <drivers/ata.h>
#include <fs/common.h>
#include <fs/ext.h>
#include <memory.h>

#define EXT2_SIG 0xef53

#define EXT2_NAME_LEN 255
#define EXT2_DIR_REC_LEN(name_len) (((name_len) + 8 + 3) & ~3u)

#define EXT2_S_IFMT 0xF000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFREG 0x8000

#define EXT2_ROOT_INO 2

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_OS_MASIX 2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES 4

/* super.c */
int ext2_mount(ext2_t *fs, partition_t *part);
int ext2_read_block(ext2_t *fs, uint32_t block, void *buf);

/* inode.c */
int64_t ext2_bmap(ext2_t *fs, const ext2_inode *ino, uint32_t i_block);
int ext2_read_inode(ext2_t *fs, uint32_t ino, ext2_inode *out);
int ext2_iget(ext2_t *fs, uint32_t ino, ext2_inode *out);

/* dir.c */
int ext2_find_entry(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, uint32_t *ino);

/* namei.c */
int ext2_lookup(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, ext2_inode *out, uint32_t *out_ino);
int ext2_namei(ext2_t *fs, const char *path, ext2_inode *out, uint32_t *out_ino);