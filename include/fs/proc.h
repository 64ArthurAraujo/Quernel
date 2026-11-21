#pragma once
#include <fs/ext.h>
int proc_write_file(ext2_t *fs, const char *path, char *name, char *buf);