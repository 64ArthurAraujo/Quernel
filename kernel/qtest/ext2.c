#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <drivers/tty.h>
#include <quernel/dev.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <fs/ext.h>

#include "qtest.h"

/**
 * test_create_find_dir():
 *
 * Creates /testdir and checks, reading everything back from disk through
 * ext2_namei, that the directory looks right: type, permissions, size, link
 * counts, "." and "..", duplicate rejection and a nested mkdir.
 */
void test_create_find_dir(root_dev sda)
{
    pr_info("test_create_find_dir: starting checks\n");

    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *fs = (ext2_t *)v.fs;
    ext2_inode *root_dir = &(fs->root_inode);

    ext2_inode inode, tmp;
    uint32_t ino, tmp_ino;
    int r;

    failed = 0;

    uint32_t root_links = root_dir->i_links_count;

    /* 1. create */
    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "testdir", 7, 0755);
    assert_that(r == 0, "mkdir /testdir returns 0");
    if (r != 0)
    {
        pr_err("mkdir returned %i, stopping\n", r);
        return;
    }

    /* 2. the new directory, read back from disk */
    r = ext2_namei(fs, "/testdir", &inode, &ino);
    assert_that(r == 0, "namei finds /testdir");
    if (r != 0)
        return;

    assert_that(S_ISDIR(inode.i_mode), "/testdir has the directory type bit");
    assert_that((inode.i_mode & 0777) == 0755, "/testdir has permissions 0755");
    assert_that(inode.i_size == fs->block_size, "/testdir size is one block");
    assert_that(inode.i_blocks == fs->block_size / 512, "/testdir i_blocks is one block");
    assert_that(inode.i_links_count == 2, "/testdir has 2 links");
    assert_that(inode.i_block[0] != 0, "/testdir has a data block");
    assert_that(inode.i_block[1] == 0, "/testdir has no garbage in i_block[1]");
    assert_that(inode.i_dtime == 0, "/testdir has i_dtime 0");

    /* 3. "." and ".." */
    r = ext2_namei(fs, "/testdir/.", &tmp, &tmp_ino);
    assert_that(r == 0 && tmp_ino == ino, "'.' points to the directory itself");

    r = ext2_namei(fs, "/testdir/..", &tmp, &tmp_ino);
    assert_that(r == 0 && tmp_ino == EXT2_ROOT_INO, "'..' points to the parent (root)");

    /* 4. the parent's link count went up, in memory and on disk */
    assert_that(root_dir->i_links_count == root_links + 1, "cached root link count +1");

    r = ext2_namei(fs, "/", &tmp, &tmp_ino);
    assert_that(r == 0 && tmp.i_links_count == root_links + 1, "on-disk root link count +1");

    /* 5. duplicate name */
    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "testdir", 7, 0755);
    assert_that(r == -EEXIST, "second mkdir /testdir returns -EEXIST");
    assert_that(root_dir->i_links_count == root_links + 1, "failed mkdir did not change root links");

    /* 6. nested directory: parent is the inode we just read */
    r = ext2_mkdir(fs, &inode, ino, "inner", 5, 0700);
    assert_that(r == 0, "mkdir /testdir/inner returns 0");

    r = ext2_namei(fs, "/testdir/inner", &tmp, &tmp_ino);
    assert_that(r == 0 && S_ISDIR(tmp.i_mode), "namei finds /testdir/inner");

    uint32_t inner_ino = tmp_ino;

    r = ext2_namei(fs, "/testdir/inner/..", &tmp, &tmp_ino);
    assert_that(r == 0 && tmp_ino == ino, "'..' of inner points to /testdir");

    r = ext2_namei(fs, "/testdir", &tmp, &tmp_ino);
    assert_that(r == 0 && tmp.i_links_count == 3, "/testdir now has 3 links");

    /* 7. mkdir on something that is not a directory must be refused */
    ext2_inode fake = inode;
    fake.i_mode = S_IFREG | 0644;
    r = ext2_mkdir(fs, &fake, ino, "nope", 4, 0755);
    assert_that(r == -ENOTDIR, "mkdir under a non-directory returns -ENOTDIR");

    /* 8. bad names */
    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "x", 0, 0755);
    assert_that(r == -ENAMETOOLONG, "mkdir with empty name is refused");

    (void)inner_ino;

    if (failed == 0)
        pr_info("test_create_find_dir: all checks passed\n");
    else
        pr_err("test_create_find_dir: %i check(s) failed\n", failed);
}

/**
 * test_create_many_folders():
 *
 * Fills /stress with enough subdirectories that its single directory block
 * overflows, which forces ext2_add_link to append a new block to the parent.
 * Directory names are d0, d1, d2, ...
 */
void test_create_many_folders(root_dev sda)
{
    pr_info("test_create_many_folders: starting checks\n");

    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *fs = (ext2_t *)v.fs;
    ext2_inode *root_dir = &(fs->root_inode);

    ext2_inode stress, tmp;
    uint32_t stress_ino, tmp_ino;
    char name[16];
    int r;

    failed = 0;

    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "stress", 6, 0755);
    assert_that(r == 0, "mkdir /stress returns 0");
    if (r != 0)
        return;

    r = ext2_namei(fs, "/stress", &stress, &stress_ino);
    assert_that(r == 0, "namei finds /stress");
    if (r != 0)
        return;

    /* a 12 byte entry fits block_size/12 times; go past that to force growth */
    uint32_t count = fs->block_size / 12 + 10;

    uint32_t created = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        make_name(name, i);
        uint32_t len = 0;
        while (name[len])
            len++;

        r = ext2_mkdir(fs, &stress, stress_ino, name, len, 0755);
        if (r != 0)
        {
            pr_err("mkdir /stress/%s failed with %i\n", name, r);
            break;
        }
        created++;
    }

    assert_that(created == count, "all subdirectories were created");

    /* reread the parent from disk */
    r = ext2_namei(fs, "/stress", &tmp, &tmp_ino);
    assert_that(r == 0, "namei finds /stress again");
    assert_that(tmp.i_size >= 2 * fs->block_size, "/stress grew to at least 2 blocks");
    assert_that(tmp.i_blocks >= 2 * (fs->block_size / 512), "/stress i_blocks grew");
    assert_that(tmp.i_links_count == 2 + created, "/stress link count is 2 + subdirectories");

    /* the first one (block 0) and the last one (second block) must be reachable */
    make_name(name, 0);
    char path[32] = "/stress/";
    int p = 8;
    for (int k = 0; name[k]; k++)
        path[p++] = name[k];
    path[p] = 0;
    r = ext2_namei(fs, path, &tmp, &tmp_ino);
    assert_that(r == 0 && S_ISDIR(tmp.i_mode), "first subdirectory is reachable");

    make_name(name, count - 1);
    p = 8;
    for (int k = 0; name[k]; k++)
        path[p++] = name[k];
    path[p] = 0;
    r = ext2_namei(fs, path, &tmp, &tmp_ino);
    assert_that(r == 0 && S_ISDIR(tmp.i_mode), "last subdirectory is reachable");

    if (failed == 0)
        pr_info("create_many_folders: all checks passed\n");
    else
        pr_err("create_many_folders: %i check(s) failed\n", failed);
}