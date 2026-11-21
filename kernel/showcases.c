/*
 * showcases.c
 *
 * This file contains some fun test scenarions to play with the Quernel
 * remember to NOT run these in bare metal because it is very likely
 * to break something important in your system.
 *
 */
#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <drivers/tty.h>
#include <quernel/dev.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <fs/ext.h>

static int failed;

static void expect(int cond, const char *msg)
{
    if (cond)
    {
        pr_info("ok:   %s\n", msg);
    }
    else
    {
        pr_err("FAIL: %s\n", msg);
        failed++;
    }
}

/* writes "d<number>" into out, no libc formatting needed */
static void make_name(char *out, uint32_t i)
{
    char tmp[12];
    int n = 0;
    do
    {
        tmp[n++] = '0' + (i % 10);
        i /= 10;
    } while (i);
    out[0] = 'd';
    for (int k = 0; k < n; k++)
        out[1 + k] = tmp[n - 1 - k];
    out[1 + n] = 0;
}


/**
 * read_hello_world():
 *
 * Reads the lyrics of the music Hello World by Louie Zong
 * stored in the rootfs folder that is packed in the iso as
 * the root filesystem of the quernel formatted in ext2.
 */
void read_hello_world(root_dev sda)
{
    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *ext2_part = (ext2_t *)v.fs;

    ext2_inode ino;
    uint32_t num;
    static char data[512];

    if (ext2_namei(ext2_part, "/hello+world.txt", &ino, &num) == 0)
    {
        int64_t n = ext2_read_file(ext2_part, &ino, 0, data, sizeof(data) - 1);

        if (n >= 0)
        {
            data[n] = 0;
            tty_emit(data);
        }
    }
}

/**
 * read_hello_world_broken():
 *
 * The scenario is the same of read_hello_world but before
 * actually reading the text file we overwrite part of the ext2 superblock
 * with zeros causing a error when we try to read the file.
 */
void read_hello_world_broken(root_dev sda)
{
    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *ext2_part = (ext2_t *)v.fs;

    uint16_t zeros[256] = {0};
    write_partition(&(ext2_part->part), (ext2_part->block_size / 512), zeros);

    read_hello_world(sda);
}

/**
 * create_and_find_file():
 *
 * This scenario is a example of how to use the quernel's ext2 filesystem implementation,
 * in here we create a file and search for it using ext2_namei
 */
void create_and_find_file(root_dev sda)
{
    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *ext2_part = (ext2_t *)v.fs;
    ext2_inode *root_dir = &(ext2_part->root_inode);

    ext2_create_file(ext2_part, root_dir, EXT2_ROOT_INO, "test", S_IFREG);

    ext2_inode inode;
    uint32_t ino;

    if (ext2_namei(ext2_part, "/test", &inode, &ino) == 0)
    {
        // if everything is correct the i_ctime should be 67, at least while we are not able to get the current unix timestamp.
        pr_debug("We found the created test file!\n i_ctime=%i", inode.i_ctime);
    }
    else
    {
        pr_err("Oops we failed to find the created test file!\n");
    }
}

/* helper of create_write_and_read_file */
void static write_read(ext2_t *fs, ext2_inode inode, uint32_t ino, char *text)
{
    if (ext2_write_file(fs, &inode, ino, 0, text, strlen(text)) < 0)
    {
        pr_err("Failed to write file!");
        return;
    }

    char data[512];
    int64_t n = ext2_read_file(fs, &inode, 0, data, sizeof(data) - 1);

    if (n >= 0)
    {
        data[n] = 0;
        tty_emit(data);
    }
}

/**
 * create_write_and_read_file():
 *
 * In this scenario we create a file, check it exists, write to it, read its content
 * and write again, repeating 4 times, reading the first 4 "paragraphs" of the portuguese
 * epic "The Lusiads" (in english because we dont deal with ´ and ~ yet)
 */
void create_write_and_read_file(root_dev sda)
{
    dev_volume_t v = sda.volumes[vol(3)];

    if (v.fs_type != EXT2_T)
    {
        pr_err("Oops, it seems that sda3 does not contain a ext2 fs! (volume.fs_type=%i)\n", v.fs_type);
        return;
    }

    ext2_t *ext2_part = (ext2_t *)v.fs;
    ext2_inode *root_dir = &(ext2_part->root_inode);

    ext2_create_file(ext2_part, root_dir, EXT2_ROOT_INO, "os+lusiadas", S_IFREG);

    ext2_inode inode;
    uint32_t ino;

    if (ext2_namei(ext2_part, "/os+lusiadas", &inode, &ino) != 0)
    {
        pr_err("namei was unable to find the file: /os+lusiadas\n");
        return;
    }

    char *estrofe_1 = "1\n\n\
Arms and the heroes signalised in fame,\n\
Who from the western Lusitanian shore\n\
Beyond e'en Taprobana ' sailing came,\n\
O'er seas that ne'er had traversed been before;\n\
Harassed with wars and dangers without name,\n\
Beyond what seemed of human prowess bore,\n\
Raised a new kingdom midst a distant clime,\n\
Which afterwards they rendered to sublime;\n\
";

    char *estrofe_2 = "2\n\n\
Also those kings of glorious memory,\n\
Who, spreading wide the faith and empire's sway.\n\
Went forth where Africa and Asia be.\n\
Sweeping the wicked of those lands away ;\n\
And they, who, working many a prodigy\n\
Of valour, death's own laws e'en held at bay.\n\
Shall in my song be o'er the world displayed,\n\
If art and genius so far lend their aid\n\
";

    char *estrofe_3 = "3\n\n\
Cease now those mighty voyages to proclaim,\n\
The Trojan and the learned Greek sustained ;\n\
No more of victories and all their fame,\n\
Which Trajan and great Alexander gained ;\n\
I sing a daring Lusitanian name.\n\
O'er Neptune and o'er Mars to rule ordained ;\n\
Cease all the Ancient Muse lo sing was wont,\n\
For other valour rears a bolder front.\n\
";

    char *estrofe_4 = "4\n\n\
And ye ! my nymphs of Tagus, since my rhyme\n\
With ardent genius new ye now inspire,\n\
If I was wont, well pleased, in former time,\n\
To celebrate your stream with humble lyre,\n\
Oh ! grant me now a lofty note sublime,\n\
An eloquent and glowing line of fire,\n\
For of your waters Phoebus doth ordain,\n\
They shall not envy those of Hippocrene !\n\
";

    write_read(ext2_part, inode, ino, estrofe_1);
    sleep(2000);
    write_read(ext2_part, inode, ino, estrofe_2);
    sleep(2000);
    write_read(ext2_part, inode, ino, estrofe_3);
    sleep(2000);
    write_read(ext2_part, inode, ino, estrofe_4);
}

/**
 * create_and_find_folder():
 *
 * Creates /testdir and checks, reading everything back from disk through
 * ext2_namei, that the directory looks right: type, permissions, size, link
 * counts, "." and "..", duplicate rejection and a nested mkdir.
 */
void create_and_find_folder(root_dev sda)
{
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
    expect(r == 0, "mkdir /testdir returns 0");
    if (r != 0)
    {
        pr_err("mkdir returned %i, stopping\n", r);
        return;
    }

    /* 2. the new directory, read back from disk */
    r = ext2_namei(fs, "/testdir", &inode, &ino);
    expect(r == 0, "namei finds /testdir");
    if (r != 0)
        return;

    expect(S_ISDIR(inode.i_mode), "/testdir has the directory type bit");
    expect((inode.i_mode & 0777) == 0755, "/testdir has permissions 0755");
    expect(inode.i_size == fs->block_size, "/testdir size is one block");
    expect(inode.i_blocks == fs->block_size / 512, "/testdir i_blocks is one block");
    expect(inode.i_links_count == 2, "/testdir has 2 links");
    expect(inode.i_block[0] != 0, "/testdir has a data block");
    expect(inode.i_block[1] == 0, "/testdir has no garbage in i_block[1]");
    expect(inode.i_dtime == 0, "/testdir has i_dtime 0");

    /* 3. "." and ".." */
    r = ext2_namei(fs, "/testdir/.", &tmp, &tmp_ino);
    expect(r == 0 && tmp_ino == ino, "'.' points to the directory itself");

    r = ext2_namei(fs, "/testdir/..", &tmp, &tmp_ino);
    expect(r == 0 && tmp_ino == EXT2_ROOT_INO, "'..' points to the parent (root)");

    /* 4. the parent's link count went up, in memory and on disk */
    expect(root_dir->i_links_count == root_links + 1, "cached root link count +1");

    r = ext2_namei(fs, "/", &tmp, &tmp_ino);
    expect(r == 0 && tmp.i_links_count == root_links + 1, "on-disk root link count +1");

    /* 5. duplicate name */
    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "testdir", 7, 0755);
    expect(r == -EEXIST, "second mkdir /testdir returns -EEXIST");
    expect(root_dir->i_links_count == root_links + 1, "failed mkdir did not change root links");

    /* 6. nested directory: parent is the inode we just read */
    r = ext2_mkdir(fs, &inode, ino, "inner", 5, 0700);
    expect(r == 0, "mkdir /testdir/inner returns 0");

    r = ext2_namei(fs, "/testdir/inner", &tmp, &tmp_ino);
    expect(r == 0 && S_ISDIR(tmp.i_mode), "namei finds /testdir/inner");

    uint32_t inner_ino = tmp_ino;

    r = ext2_namei(fs, "/testdir/inner/..", &tmp, &tmp_ino);
    expect(r == 0 && tmp_ino == ino, "'..' of inner points to /testdir");

    r = ext2_namei(fs, "/testdir", &tmp, &tmp_ino);
    expect(r == 0 && tmp.i_links_count == 3, "/testdir now has 3 links");

    /* 7. mkdir on something that is not a directory must be refused */
    ext2_inode fake = inode;
    fake.i_mode = S_IFREG | 0644;
    r = ext2_mkdir(fs, &fake, ino, "nope", 4, 0755);
    expect(r == -ENOTDIR, "mkdir under a non-directory returns -ENOTDIR");

    /* 8. bad names */
    r = ext2_mkdir(fs, root_dir, EXT2_ROOT_INO, "x", 0, 0755);
    expect(r == -ENAMETOOLONG, "mkdir with empty name is refused");

    (void)inner_ino;

    if (failed == 0)
        pr_info("create_and_find_folder: all checks passed\n");
    else
        pr_err("create_and_find_folder: %i check(s) failed\n", failed);
}