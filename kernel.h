#ifndef KERNEL_H
#define KERNEL_H

#include "fs.h"
#include <stdbool.h>

typedef struct
{
    int inode_id;
    uint32_t offset; // Зміщення для наступного читання/запису [cite: 156, 278]
    bool is_open;
} fd_entry_t;

void kernel_init();
void k_mkfs();
void k_stat(const char *name);
void k_ls();
void k_create(const char *name);
int k_open(const char *name);
void k_close(int fd);
void k_seek(int fd, int offset);
int k_read(int fd, int size);
int k_write(int fd, int size, const char *data);
void k_link(const char *name1, const char *name2);
void k_unlink(const char *name);
void k_truncate(const char *name, int size);

#endif