#include <quernel/printk.h>
#include <quernel/config.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <drivers/tty.h>
#include <quernel/dev.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <fs/ext.h>

#include "qtest.h"

static inline void pr_result(char *t)
{
    if (failed == 0)
        pr_info("[OK] %s\n", t);
    else
        pr_err("[ER] %s: %i check(s) failed\n", t, failed);
}

/**
 * test_filesys():
 *
 * Checks if the filesystem contains sane information and
 * is not broken/corrupt.
 */
int test_filesys(ext2_t *fs)
{
    ext2_super_block sb = fs->sb;

    failed = 0;

    assert_that(sb.signature == EXT2_SIG, "signature is 0xef53");
    assert_that(fs->block_size == (1024u << fs->sb.log_block_size), "block_size follows log_block_size");
    assert_that(fs->group_count >= 1 && fs->group_count <= EXT2_MAX_GROUPS, "group_count is within a valid range");
    assert_that(fs->group_count == (fs->sb.blocks_count + fs->sb.blocks_per_group - 1) / fs->sb.blocks_per_group, "group_count matches blocks_count / blocks_per_group");
    assert_that(fs->inode_size >= 128, "inode_size is >= 128");
    assert_that(fs->bgd != NULL, "block group descriptor is successfully read");
    assert_that(S_ISDIR(fs->root_inode.i_mode), "root_inode is a dir");

    ext2_t *fs2 = kmalloc(sizeof(ext2_t));

    assert_that(ext2_mount(fs2, &fs->part) == 0, "remount the filesystem");

    if (fs2->bgd)
    {
        assert_that(fs2->block_size == fs->block_size, "remounted fs's block_size is equal to the original");
        assert_that(fs2->group_count == fs->group_count, "remounted fs's group_count is equal to the original");
        assert_that(fs2->sb.free_inodes_count == fs->sb.free_inodes_count, "remounted super_block's free_inodes_count is equal to the original");
        assert_that(fs2->sb.free_blocks_count == fs->sb.free_blocks_count, "remounted super_block's free_blocks_count is equal to the original");
        assert_that(memcmp(fs2->bgd, fs->bgd, fs->group_count * sizeof(ext2_group_desc)) == 0, "remounted block group descriptor is equal to the original");
    }

    kfree(fs2);
    pr_result("test_filesys");
    return failed;
}

/**
 * test_write_sb_bgd():
 *
 * Checks if writing the super_block and the
 * block group descriptor works correctly.
 */
int test_write_sb_bgd(ext2_t *fs)
{
    ext2_super_block sb = fs->sb;

    failed = 0;

    assert_that(ext2_write_super_block(fs) == 0, "writing the superblock works");

    for (uint32_t group = 0; group < fs->group_count; group++)
    {
        assert_that(ext2_write_bgdt(fs, group) == 0, "writing group works");
    }

    ext2_t *fs2 = kmalloc(sizeof(ext2_t));

    assert_that(ext2_mount(fs2, &fs->part) == 0, "remount the filesystem after writing");

    if (fs2->bgd)
    {
        assert_that(memcmp(&(fs2->sb), &(fs->sb), sizeof(ext2_super_block)) == 0, "superblock is unchanged");
        assert_that(memcmp(fs2->bgd, fs->bgd, sizeof(ext2_group_desc)) == 0, "block group descriptor is unchanged");
    }

    kfree(fs2);

    pr_result("test_write_sb_bgd");
    return failed;
}

/**
 * test_create_find_dir():
 *
 * Creates /testdir and checks, reading everything back from disk through
 * ext2_namei, that the directory looks right: type, permissions, size, link
 * counts, "." and "..", duplicate rejection and a nested mkdir.
 */
int test_create_find_dir(ext2_t *fs)
{
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
        pr_err("mkdir returned %s, stopping\n", errno_str(r));
        return failed;
    }

    /* 2. the new directory, read back from disk */
    r = ext2_namei(fs, "/testdir", &inode, &ino);
    assert_that(r == 0, "namei finds /testdir");
    if (r != 0)
        return failed;

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

    pr_result("test_create_find_dir");
    return failed;
}

/**
 * test_create_many_folders():
 *
 * Fills /stress with enough subdirectories that its single directory block
 * overflows, which forces ext2_add_link to append a new block to the parent.
 * Directory names are d0, d1, d2, ...
 */
int test_create_many_folders(ext2_t *fs)
{
    ext2_inode *root_dir = &(fs->root_inode);

    ext2_inode stress, tmp;
    uint32_t stress_ino, tmp_ino;
    char name[16];
    int r;

    failed = 0;

    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "stress", 6, 0755);
    assert_that(r == 0, "mkdir /stress returns 0");
    if (r != 0)
        return failed;

    r = ext2_namei(fs, "/stress", &stress, &stress_ino);
    assert_that(r == 0, "namei finds /stress");
    if (r != 0)
        return failed;

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
            pr_err("mkdir /stress/%s failed with %s\n", name, errno_str(r));
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

    pr_result("test_create_many_folders");
    return failed;
}

int test_new_inode(ext2_t *fs)
{
    ext2_inode *root_dir = &(fs->root_inode);

    failed = 0;

    // the number of free inodes before a new inode
    uint32_t prev_free_inodes_count = fs->sb.free_inodes_count;

    ext2_inode *f_inode = ext2_new_inode(fs, root_dir, EXT2_ROOT_INO, S_IFREG | 0644);
    assert_that(f_inode != NULL, "ext2_new_inode returns a non-null pointer");

    if (!f_inode)
        return failed;

    uint32_t ino = f_inode->osd1.masix1.m_i_reserved1;

    assert_that(ino >= first_ino(fs) && ino <= fs->sb.inodes_count, "new inode number is within a reasonable range");
    assert_that(f_inode->i_mode == (S_IFREG | 0644), "inode->i_mode is correctly assigned");
    assert_that(f_inode->i_links_count == 1, "inode links count is 1");
    assert_that(f_inode->i_size == 0, "i_size is zero");
    assert_that(f_inode->i_blocks == 0, "i_blocks is zero");
    assert_that(f_inode->i_flags == 0 && f_inode->i_file_acl == 0 && f_inode->i_generation == 0, "flags/acl/generation are zeroed");

    int blocks_zero = 1;
    for (int i = 0; i < EXT2_N_BLOCKS; i++)
        if (f_inode->i_block[i])
            blocks_zero = 0;
    assert_that(blocks_zero, "i_block[] is zeroed");

    assert_that(fs->sb.free_inodes_count == (prev_free_inodes_count - 1), "super_block's free_inodes_count decreased by one");

    pr_result("test_new_inode");
    return failed;
}

/**
 * test_ext2_fs():
 *
 * Entrypoint that runs all unit tests, returning 0 if all
 * passed and 1 if something failed. Execution of the next
 * test is stopped if the current test fails.
 *
 * NOTE:
 * If QUER_QTEST is disabled this function will return 0 instantly.
 */
int test_ext2_fs(ext2_t *fs)
{
#if QUER_QTEST
    int t_fs = test_filesys(fs);
    if (t_fs > 0)
        return 1;
    int t_wr_sb_bgd = test_write_sb_bgd(fs);
    if (t_wr_sb_bgd > 0)
        return 1;
    int t_c_f_dir = test_create_find_dir(fs);
    if (t_c_f_dir > 0)
        return 1;
    int t_c_m_folders = test_create_many_folders(fs);
    if (t_c_m_folders > 0)
        return 1;
    int t_n_inode = test_new_inode(fs);
    if (t_n_inode > 0)
        return 1;
#endif
    return 0;
}