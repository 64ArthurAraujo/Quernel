#pragma once
#include <stdint.h>
#include <drivers/ata.h>
#include <fs/common.h>
#include <fs/ext.h>
#include <memory.h>

#define EXT2_SIG 0xef53

#define EXT2_NAME_LEN 255
#define EXT2_DIR_REC_LEN(name_len) (((name_len) + 8 + 3) & ~3u)

#define EXT2_MAX_BLOCK_SIZE 4096

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
int ext2_write_block(ext2_t *fs, uint32_t block, uint16_t *buf);
int ext2_write_bgdt(ext2_t *fs, uint32_t group);
int ext2_write_super_block(ext2_t *fs);

/* inode.c */
int64_t ext2_bmap(ext2_t *fs, const ext2_inode *ino, uint32_t i_block);
int ext2_read_inode(ext2_t *fs, uint32_t ino, ext2_inode *out);
int ext2_iget(ext2_t *fs, uint32_t ino, ext2_inode *out);
int ext2_write_inode(ext2_t *fs, uint32_t ino, ext2_inode *inode);

/* dir.c */
int dir_block_has_name(uint8_t *buf, uint32_t limit, const char *name, uint8_t name_len);
int ext2_check_dir_block(ext2_t *fs, const uint8_t *buf, uint32_t limit);
int ext2_find_entry(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, uint32_t *ino);
int dir_block_insert(uint8_t *buf, uint32_t limit, uint32_t ino, const char *name, uint8_t name_len, uint8_t f_type);

/* namei.c */
int ext2_lookup(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, ext2_inode *out, uint32_t *out_ino);
int ext2_namei(ext2_t *fs, const char *path, ext2_inode *out, uint32_t *out_ino);

/* file.c */
int64_t ext2_read_file(ext2_t *fs, const ext2_inode *ino, uint32_t offset, void *dst, uint32_t count);

/* ialloc.c */
ext2_inode *ext2_new_inode(ext2_t *fs, ext2_inode *dir, uint32_t dir_ino, uint16_t mode);