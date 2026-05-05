#include "kernel.h"
#include <stdio.h>
#include <string.h>

fd_entry_t fd_table[MAX_FD];
int cwd_inode = 0;

int get_free_inode()
{
    for (int i = 1; i < MAX_FILES; i++)
    {
        if (inodes[i].type == TYPE_EMPTY)
            return i;
    }
    return -1;
}

int dir_find_entry(int dir_inode, const char *name)
{
    inode_t *dir = &inodes[dir_inode];
    for (uint32_t offset = 0; offset < dir->size; offset += sizeof(dir_entry_t))
    {
        int phys_blk = fs_get_phys_block(dir_inode, offset / BLOCK_SIZE, 0);
        if (phys_blk == -1)
            continue;
        dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][offset % BLOCK_SIZE];
        if (entry->inode_id != -1 && strcmp(entry->name, name) == 0)
            return entry->inode_id;
    }
    return -1;
}

int dir_add_entry(int dir_inode, const char *name, int inode_id)
{
    inode_t *dir = &inodes[dir_inode];
    for (uint32_t offset = 0; offset < dir->size; offset += sizeof(dir_entry_t))
    {
        int phys_blk = fs_get_phys_block(dir_inode, offset / BLOCK_SIZE, 1);
        dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][offset % BLOCK_SIZE];
        if (entry->inode_id == -1)
        {
            strncpy(entry->name, name, MAX_NAME_LEN);
            entry->inode_id = inode_id;
            return 0;
        }
    }
    int phys_blk = fs_get_phys_block(dir_inode, dir->size / BLOCK_SIZE, 1);
    if (phys_blk == -1)
        return -1;
    dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][dir->size % BLOCK_SIZE];
    strncpy(entry->name, name, MAX_NAME_LEN);
    entry->inode_id = inode_id;
    dir->size += sizeof(dir_entry_t);
    return 0;
}

void dir_remove_entry(int dir_inode, const char *name)
{
    inode_t *dir = &inodes[dir_inode];
    for (uint32_t offset = 0; offset < dir->size; offset += sizeof(dir_entry_t))
    {
        int phys_blk = fs_get_phys_block(dir_inode, offset / BLOCK_SIZE, 0);
        if (phys_blk == -1)
            continue;
        dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][offset % BLOCK_SIZE];
        if (entry->inode_id != -1 && strcmp(entry->name, name) == 0)
        {
            entry->inode_id = -1;
            return;
        }
    }
}

bool dir_is_empty(int dir_inode)
{
    inode_t *dir = &inodes[dir_inode];
    for (uint32_t offset = 0; offset < dir->size; offset += sizeof(dir_entry_t))
    {
        int phys_blk = fs_get_phys_block(dir_inode, offset / BLOCK_SIZE, 0);
        if (phys_blk == -1)
            continue;
        dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][offset % BLOCK_SIZE];
        if (entry->inode_id != -1 && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0)
        {
            return false;
        }
    }
    return true;
}

int resolve_path_base(const char *path, int base_dir, int *parent_id, char *name_out, int follow_last, int hops)
{
    if (hops > MAX_SYMLINK_HOPS)
        return -1;
    if (!path || path[0] == '\0')
        return base_dir;

    int curr_dir = (path[0] == '/') ? 0 : base_dir;
    char p[256];
    strncpy(p, path, 256);

    char *saveptr;
    char *token = strtok_r(p, "/", &saveptr);
    int prev_dir = curr_dir;

    while (token != NULL)
    {
        char *next_token = strtok_r(NULL, "/", &saveptr);
        bool is_last = (next_token == NULL);

        int next_inode = dir_find_entry(curr_dir, token);
        if (next_inode == -1)
        {
            if (is_last && parent_id && name_out)
            {
                *parent_id = curr_dir;
                strcpy(name_out, token);
                return -1;
            }
            return -1;
        }

        if (inodes[next_inode].type == TYPE_SYM && (!is_last || follow_last))
        {
            char sym_path[BLOCK_SIZE + 1] = {0};
            int phys_blk = fs_get_phys_block(next_inode, 0, 0);
            if (phys_blk != -1)
                strncpy(sym_path, (char *)disk[phys_blk], inodes[next_inode].size);

            char new_path[512] = {0};
            strcpy(new_path, sym_path);
            if (!is_last && saveptr && *saveptr != '\0')
            {
                strcat(new_path, "/");
                strcat(new_path, saveptr);
            }
            int sym_base = (sym_path[0] == '/') ? 0 : curr_dir;
            return resolve_path_base(new_path, sym_base, parent_id, name_out, follow_last, hops + 1);
        }

        if (!is_last && inodes[next_inode].type != TYPE_DIR)
            return -1;

        prev_dir = curr_dir;
        curr_dir = next_inode;

        if (is_last)
        {
            if (parent_id)
                *parent_id = prev_dir;
            if (name_out)
                strcpy(name_out, token);
            return curr_dir;
        }
        token = next_token;
    }

    if (parent_id)
        *parent_id = curr_dir;
    if (name_out)
        strcpy(name_out, ".");
    return curr_dir;
}

void kernel_init()
{
    for (int i = 0; i < MAX_FD; i++)
        fd_table[i].is_open = false;
    fs_init();

    inodes[0].type = TYPE_DIR;
    inodes[0].nlink = 2;
    inodes[0].size = 0;
    dir_add_entry(0, ".", 0);
    dir_add_entry(0, "..", 0);
    cwd_inode = 0;
}

void k_mkfs()
{
    kernel_init();
    printf("FS initialized.\n");
}

void k_mkdir(const char *path)
{
    int parent;
    char name[MAX_NAME_LEN];
    int target = resolve_path_base(path, cwd_inode, &parent, name, 0, 0);
    if (target != -1)
    {
        printf("Already exists\n");
        return;
    }
    if (parent == -1)
    {
        printf("No such file or directory\n");
        return;
    }

    int id = get_free_inode();
    if (id == -1)
        return;

    inodes[id].type = TYPE_DIR;
    inodes[id].nlink = 2;
    inodes[id].size = 0;

    dir_add_entry(id, ".", id);
    dir_add_entry(id, "..", parent);
    dir_add_entry(parent, name, id);
    inodes[parent].nlink++;
}

void k_rmdir(const char *path)
{
    int parent;
    char name[MAX_NAME_LEN];
    int id = resolve_path_base(path, cwd_inode, &parent, name, 0, 0);
    if (id == -1 || inodes[id].type != TYPE_DIR)
    {
        printf("No such directory\n");
        return;
    }
    if (id == 0)
        return;
    if (!dir_is_empty(id))
    {
        printf("Not empty\n");
        return;
    }

    dir_remove_entry(parent, name);
    inodes[parent].nlink--;

    for (int j = 0; j < DIRECT_BLOCKS; j++)
        if (inodes[id].direct[j] != -1)
            fs_free_block(inodes[id].direct[j]);
    if (inodes[id].indirect != -1)
        fs_free_block(inodes[id].indirect);
    inodes[id].type = TYPE_EMPTY;
}

void k_cd(const char *path)
{
    int id = resolve_path_base(path, cwd_inode, NULL, NULL, 1, 0);
    if (id != -1 && inodes[id].type == TYPE_DIR)
        cwd_inode = id;
    else
        printf("No such directory\n");
}

void k_symlink(const char *str, const char *path)
{
    int parent;
    char name[MAX_NAME_LEN];
    int target = resolve_path_base(path, cwd_inode, &parent, name, 0, 0);
    if (target != -1)
    {
        printf("Already exists\n");
        return;
    }
    if (parent == -1)
        return;

    int id = get_free_inode();
    if (id == -1)
        return;

    inodes[id].type = TYPE_SYM;
    inodes[id].nlink = 1;
    inodes[id].size = strlen(str);

    int phys_blk = fs_get_phys_block(id, 0, 1);
    strncpy((char *)disk[phys_blk], str, BLOCK_SIZE);
    dir_add_entry(parent, name, id);
}

void k_ls(const char *path)
{
    int target = cwd_inode;
    if (path && strlen(path) > 0)
        target = resolve_path_base(path, cwd_inode, NULL, NULL, 1, 0);
    if (target == -1 || inodes[target].type != TYPE_DIR)
        return;

    inode_t *dir = &inodes[target];
    for (uint32_t offset = 0; offset < dir->size; offset += sizeof(dir_entry_t))
    {
        int phys_blk = fs_get_phys_block(target, offset / BLOCK_SIZE, 0);
        if (phys_blk == -1)
            continue;
        dir_entry_t *entry = (dir_entry_t *)&disk[phys_blk][offset % BLOCK_SIZE];
        if (entry->inode_id != -1)
        {
            inode_t *e = &inodes[entry->inode_id];
            if (e->type == TYPE_DIR)
                printf("%s\t=> dir, %d\n", entry->name, entry->inode_id);
            else if (e->type == TYPE_REG)
                printf("%s\t=> reg, %d\n", entry->name, entry->inode_id);
            else if (e->type == TYPE_SYM)
            {
                char sym_t[129] = {0};
                int pb = fs_get_phys_block(entry->inode_id, 0, 0);
                if (pb != -1)
                    strncpy(sym_t, (char *)disk[pb], e->size);
                printf("%s\t=> sym, %d -> %s\n", entry->name, entry->inode_id, sym_t);
            }
        }
    }
}

void k_create(const char *path)
{
    int parent;
    char name[MAX_NAME_LEN];
    int target = resolve_path_base(path, cwd_inode, &parent, name, 0, 0);
    if (target != -1)
        return;
    if (parent == -1)
    {
        printf("No such file or directory\n");
        return;
    }

    int id = get_free_inode();
    if (id == -1)
        return;

    inodes[id].type = TYPE_REG;
    inodes[id].nlink = 1;
    inodes[id].size = 0;
    dir_add_entry(parent, name, id);
}

int k_open(const char *path)
{
    int id = resolve_path_base(path, cwd_inode, NULL, NULL, 1, 0);
    if (id == -1)
    {
        printf("No such file or directory\n");
        return -1;
    }

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

void k_stat(const char *path)
{
    int id = resolve_path_base(path, cwd_inode, NULL, NULL, 0, 0);
    if (id == -1)
        return;
    inode_t n = inodes[id];
    int blocks = 0;
    for (int i = 0; i < DIRECT_BLOCKS; i++)
        if (n.direct[i] != -1)
            blocks++;
    const char *t_str = (n.type == TYPE_REG) ? "reg" : (n.type == TYPE_DIR) ? "dir"
                                                                            : "sym";
    printf("id=%d, type=%s, nlink=%d, size=%u, nblock=%d\n", id, t_str, n.nlink, n.size, blocks);
}

void k_link(const char *path1, const char *path2)
{
    int id = resolve_path_base(path1, cwd_inode, NULL, NULL, 0, 0);
    if (id == -1 || inodes[id].type == TYPE_DIR)
        return;

    int p2;
    char n2[MAX_NAME_LEN];
    int tgt = resolve_path_base(path2, cwd_inode, &p2, n2, 0, 0);
    if (tgt != -1 || p2 == -1)
        return;

    dir_add_entry(p2, n2, id);
    inodes[id].nlink++;
}

void k_unlink(const char *path)
{
    int parent;
    char name[MAX_NAME_LEN];
    int id = resolve_path_base(path, cwd_inode, &parent, name, 0, 0);
    if (id == -1)
        return;
    if (inodes[id].type == TYPE_DIR)
    {
        printf("Not allowed\n");
        return;
    }

    dir_remove_entry(parent, name);
    inodes[id].nlink--;

    if (inodes[id].nlink == 0)
    {
        bool open = false;
        for (int i = 0; i < MAX_FD; i++)
            if (fd_table[i].is_open && fd_table[i].inode_id == id)
                open = true;
        if (!open)
        {
            for (int j = 0; j < DIRECT_BLOCKS; j++)
                if (inodes[id].direct[j] != -1)
                    fs_free_block(inodes[id].direct[j]);
            if (inodes[id].indirect != -1)
                fs_free_block(inodes[id].indirect);
            inodes[id].type = TYPE_EMPTY;
        }
    }
}

void k_close(int fd)
{
    if (fd >= 0 && fd < MAX_FD)
    {
        int id = fd_table[fd].inode_id;
        fd_table[fd].is_open = false;
        if (inodes[id].nlink == 0)
        {
            for (int j = 0; j < DIRECT_BLOCKS; j++)
                if (inodes[id].direct[j] != -1)
                    fs_free_block(inodes[id].direct[j]);
            if (inodes[id].indirect != -1)
                fs_free_block(inodes[id].indirect);
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
        int phys_blk = fs_get_phys_block(id, fd_table[fd].offset / BLOCK_SIZE, 1);
        if (phys_blk == -1)
            break;
        disk[phys_blk][fd_table[fd].offset % BLOCK_SIZE] = data[i];
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
        int phys_blk = fs_get_phys_block(id, fd_table[fd].offset / BLOCK_SIZE, 0);
        if (phys_blk == -1)
            putchar('0');
        else
            putchar(disk[phys_blk][fd_table[fd].offset % BLOCK_SIZE]);
        fd_table[fd].offset++;
    }
    putchar('\n');
    return i;
}

void k_truncate(const char *path, int size)
{
    int id = resolve_path_base(path, cwd_inode, NULL, NULL, 1, 0);
    if (id != -1 && inodes[id].type == TYPE_REG)
        inodes[id].size = size;
}

void k_seek(int fd, int off)
{
    if (fd >= 0 && fd < MAX_FD)
        fd_table[fd].offset = off;
}