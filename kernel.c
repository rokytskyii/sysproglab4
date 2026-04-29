#include "kernel.h"
#include <stdio.h>
#include <string.h>

fd_entry_t fd_table[MAX_FD];

void kernel_init()
{
    for (int i = 0; i < MAX_FD; i++)
        fd_table[i].is_open = false;
    fs_init();
}

void k_mkfs()
{
    kernel_init();
    printf("FS initialized.\n");
}

void k_ls()
{
    for (int i = 0; i < MAX_FILES; i++)
        if (root_dir[i].inode_id != -1)
            printf("%s\t%d\n", root_dir[i].name, root_dir[i].inode_id);
}

void k_create(const char *name)
{
    if (strlen(name) >= MAX_NAME_LEN)
        return;
    int id = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (inodes[i].type == TYPE_EMPTY)
        {
            id = i;
            break;
        }
    if (id == -1)
        return;

    inodes[id].type = TYPE_REG;
    inodes[id].nlink = 1;
    inodes[id].size = 0;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (root_dir[i].inode_id == -1)
        {
            strncpy(root_dir[i].name, name, MAX_NAME_LEN);
            root_dir[i].inode_id = id;
            return;
        }
    }
}

int k_open(const char *name)
{
    int id = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (root_dir[i].inode_id != -1 && strcmp(root_dir[i].name, name) == 0)
        {
            id = root_dir[i].inode_id;
            break;
        }
    if (id == -1)
        return -1;

    for (int i = 0; i < MAX_FD; i++)
    {
        if (!fd_table[i].is_open)
        {
            fd_table[i].inode_id = id;
            fd_table[i].offset = 0;
            fd_table[i].is_open = true;
            return i;
        }
    }
    return -1;
}

void k_close(int fd)
{
    if (fd >= 0 && fd < MAX_FD)
    {
        int id = fd_table[fd].inode_id;
        fd_table[fd].is_open = false;
        bool still_open = false;
        for (int i = 0; i < MAX_FD; i++)
            if (fd_table[i].is_open && fd_table[i].inode_id == id)
                still_open = true;
        if (inodes[id].nlink == 0 && !still_open)
        {
            for (int j = 0; j < DIRECT_BLOCKS; j++)
                if (inodes[id].direct[j] != -1)
                    fs_free_block(inodes[id].direct[j]);
            inodes[id].type = TYPE_EMPTY;
        }
    }
}

int k_write(int fd, int size, const char *data)
{
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].is_open)
        return -1;
    int id = fd_table[fd].inode_id;
    int i = 0;
    for (i = 0; i < size; i++)
    {
        int log_blk = (fd_table[fd].offset) / BLOCK_SIZE;
        int blk_off = (fd_table[fd].offset) % BLOCK_SIZE;
        int phys_blk = fs_get_phys_block(id, log_blk, 1);
        if (phys_blk == -1)
            break;
        disk[phys_blk][blk_off] = data[i];
        fd_table[fd].offset++;
    }
    if (fd_table[fd].offset > inodes[id].size)
        inodes[id].size = fd_table[fd].offset;
    return i;
}

int k_read(int fd, int size)
{
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd].is_open)
        return -1;
    int id = fd_table[fd].inode_id;
    if (fd_table[fd].offset + size > inodes[id].size)
        size = inodes[id].size - fd_table[fd].offset;
    int i = 0;
    for (i = 0; i < size; i++)
    {
        int log_blk = (fd_table[fd].offset) / BLOCK_SIZE;
        int blk_off = (fd_table[fd].offset) % BLOCK_SIZE;
        int phys_blk = fs_get_phys_block(id, log_blk, 0);
        if (phys_blk == -1)
            putchar('0');
        else
            putchar(disk[phys_blk][blk_off]);
        fd_table[fd].offset++;
    }
    putchar('\n');
    return i;
}

void k_stat(const char *name)
{
    int id = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (root_dir[i].inode_id != -1 && strcmp(root_dir[i].name, name) == 0)
        {
            id = root_dir[i].inode_id;
            break;
        }
    if (id == -1)
        return;
    inode_t n = inodes[id];
    int blocks = 0;
    for (int i = 0; i < DIRECT_BLOCKS; i++)
        if (n.direct[i] != -1)
            blocks++;
    printf("id=%d, type=%s, nlink=%d, size=%u, nblock=%d\n", id, (n.type == TYPE_REG ? "reg" : "dir"), n.nlink, n.size, blocks);
}

void k_link(const char *n1, const char *n2)
{
    int id = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (root_dir[i].inode_id != -1 && strcmp(root_dir[i].name, n1) == 0)
            id = root_dir[i].inode_id;
    if (id == -1)
        return;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (root_dir[i].inode_id == -1)
        {
            strncpy(root_dir[i].name, n2, MAX_NAME_LEN);
            root_dir[i].inode_id = id;
            inodes[id].nlink++;
            return;
        }
    }
}

void k_unlink(const char *name)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (root_dir[i].inode_id != -1 && strcmp(root_dir[i].name, name) == 0)
        {
            int id = root_dir[i].inode_id;
            root_dir[i].inode_id = -1;
            inodes[id].nlink--;
            return;
        }
    }
}

void k_truncate(const char *name, int size)
{
    int id = -1;
    for (int i = 0; i < MAX_FILES; i++)
        if (root_dir[i].inode_id != -1 && strcmp(root_dir[i].name, name) == 0)
            id = root_dir[i].inode_id;
    if (id != -1)
        inodes[id].size = size;
}

void k_seek(int fd, int off)
{
    if (fd >= 0 && fd < MAX_FD)
        fd_table[fd].offset = off;
}