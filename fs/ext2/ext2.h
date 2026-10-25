#pragma once
#include <stdint.h>
#include <fs/common.h>
#include <fs/ext.h>

#define EXT2_SIG 0xef53

#define EXT2_NAME_LEN 255
#define EXT2_DIR_REC_LEN(name_len) (((name_len) + 8 + 3) & ~3u)

#define EXT2_S_IFMT   0xF000
#define EXT2_S_IFDIR  0x4000
#define EXT2_S_IFREG  0x8000

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_OS_MASIX 2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES 4