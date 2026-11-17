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


/**
 * read_hello_world():
 *
 * Reads the lyrics of the music Hello World by Louie Zong
 * stored in the rootfs folder that is packed in the iso as
 * the root filesystem of the quernel formatted in ext2.
 */
void read_hello_world(root_dev sda)
{
    ext2_t *ext2_part = &(sda.ext2_part);

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
    ext2_t *ext2_part = &(sda.ext2_part);

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
    ext2_t *ext2_part = &(sda.ext2_part);
    ext2_inode *root_dir = &(ext2_part->root_inode);

    ext2_create_file(ext2_part, root_dir, EXT2_ROOT_INO, "test", EXT2_S_IFREG);

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
    ext2_t *ext2_part = &(sda.ext2_part);
    ext2_inode *root_dir = &(ext2_part->root_inode);

    ext2_create_file(ext2_part, root_dir, EXT2_ROOT_INO, "os+lusiadas", EXT2_S_IFREG);

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