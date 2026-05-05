#ifndef KERNEL_H
#define KERNEL_H

#include "fs.h"
#include <stdbool.h>

typedef struct
{
    int inode_id;
    uint32_t offset;
    bool is_open;
} fd_entry_t;

void kernel_init();
void k_mkfs();
void k_stat(const char *path);
void k_ls(const char *path);
void k_create(const char *path);
int k_open(const char *path);
void k_close(int fd);
void k_seek(int fd, int offset);
int k_read(int fd, int size);
int k_write(int fd, int size, const char *data);
void k_link(const char *path1, const char *path2);
void k_unlink(const char *path);
void k_truncate(const char *path, int size);
void k_mkdir(const char *path);
void k_rmdir(const char *path);
void k_cd(const char *path);
void k_symlink(const char *str, const char *path);

#endif