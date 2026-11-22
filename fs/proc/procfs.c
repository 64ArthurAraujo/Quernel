#include <quernel/printk.h>
#include <stdint.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <drivers/ata.h>
#include <fs/ext.h>

int init_procfs(ext2_t *fs)
{
    int r = ext2_mkdir(fs, &(fs->root_inode), EXT2_ROOT_INO, "proc", 4, 0755);

    if (r == -EEXIST)
    {
        ext2_inode *inode = kmalloc(sizeof(ext2_inode));
        uint32_t ino = 0;

        if (ext2_namei(fs, "/proc", inode, &ino) != 0)
        {
            pr_err("init_procfs: Failed to retrieve information about the proc folder!\n");
            kfree(inode);
            return -1;
        }

        // This is very unlikely (but it may happen some day)
        if (!S_ISDIR(inode->i_mode))
        {
            pr_err("init_procfs: proc already exists but it is not a folder!\n");
            kfree(inode);
            return -1;
        }

        kfree(inode);
        r = 0;
    }

    if (r < 0)
    {
        pr_err("init_procfs: Failed to create the proc folder! (%s)\n", errno_str(r));
        return -1;
    }

    return 0;
}

/**
 * proc_write_file():
 *
 * Writes a proc file, creating one if it does not exist already,
 * returns 0 if created and -1 if it failed.
 *
 * @fs: The ext2 filesystem
 * @path: The path of the proc file (including /proc)
 * @name: The name of the proc file
 * @buf: Content to be written into the file
 *
 */
int proc_write_file(ext2_t *fs, const char *path, char *name, char *buf)
{
    if (init_procfs(fs) != 0)
        return -1;

    ext2_inode *proc_inode = kmalloc(sizeof(ext2_inode));
    uint32_t proc_ino = -1;

    ext2_namei(fs, "/proc", proc_inode, &proc_ino);

    ext2_inode *inode = kmalloc(sizeof(ext2_inode));
    uint32_t ino = -1;

    if (ext2_namei(fs, path, inode, &ino) != 0)
    {
        int rf = ext2_create_file(fs, proc_inode, proc_ino, name, S_IFDIR);

        if (rf < 0)
        {
            pr_err("Failed to create the proc file! (%s)\n", errno_str(rf));
            kfree(proc_inode);
            kfree(inode);
            return -1;
        }

        ext2_namei(fs, path, inode, &ino);
    }

    int r = ext2_write_file(fs, inode, ino, 0, buf, strlen(buf));

    if (r < 0)
    {
        pr_err("Failed to write the proc file! (%s)\n", errno_str(r));
        kfree(proc_inode);
        kfree(inode);
        return -1;
    }

    kfree(proc_inode);
    kfree(inode);
    return 0;
}