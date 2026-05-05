#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kernel.h"

int main()
{
    kernel_init();
    char line[256];
    printf("FS Driver v2. Use: mkdir, rmdir, cd, symlink, create, ls, stat, open, write, read, link, unlink, truncate, seek, close, exit\n");

    while (printf("> ") && fgets(line, sizeof(line), stdin))
    {
        char cmd[32] = {0}, a1[64] = {0}, a2[64] = {0}, a3[64] = {0};
        int args = sscanf(line, "%s %s %s %s", cmd, a1, a2, a3);
        if (args < 1)
            continue;

        if (!strcmp(cmd, "exit"))
            break;

        if (!strcmp(cmd, "mkdir") && args >= 2)
            k_mkdir(a1);
        else if (!strcmp(cmd, "rmdir") && args >= 2)
            k_rmdir(a1);
        else if (!strcmp(cmd, "cd") && args >= 2)
            k_cd(a1);
        else if (!strcmp(cmd, "symlink") && args >= 3)
            k_symlink(a1, a2);

        else if (!strcmp(cmd, "ls"))
        {
            if (args >= 2)
                k_ls(a1);
            else
                k_ls("");
        }
        else if (!strcmp(cmd, "create") && args >= 2)
            k_create(a1);
        else if (!strcmp(cmd, "stat") && args >= 2)
            k_stat(a1);
        else if (!strcmp(cmd, "open") && args >= 2)
            printf("fd = %d\n", k_open(a1));
        else if (!strcmp(cmd, "link") && args >= 3)
            k_link(a1, a2);
        else if (!strcmp(cmd, "unlink") && args >= 2)
            k_unlink(a1);
        else if (!strcmp(cmd, "truncate") && args >= 3)
            k_truncate(a1, atoi(a2));

        else if (!strcmp(cmd, "write") && args >= 4)
            k_write(atoi(a1), atoi(a2), a3);
        else if (!strcmp(cmd, "read") && args >= 3)
            k_read(atoi(a1), atoi(a2));
        else if (!strcmp(cmd, "seek") && args >= 3)
            k_seek(atoi(a1), atoi(a2));
        else if (!strcmp(cmd, "close") && args >= 2)
            k_close(atoi(a1));
    }
    return 0;
}