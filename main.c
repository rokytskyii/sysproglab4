#include <stdio.h>
#include <string.h>
#include "kernel.h"

int main()
{
    kernel_init();
    char cmd[32], a1[32], a2[32];
    int v1, v2;
    printf("FS Driver. Use: create, ls, stat, open, write, read, link, unlink, truncate, seek, close, exit\n");

    while (printf("> ") && scanf("%s", cmd) != EOF)
    {
        if (!strcmp(cmd, "exit"))
            break;
        if (!strcmp(cmd, "create"))
        {
            scanf("%s", a1);
            k_create(a1);
        }
        else if (!strcmp(cmd, "ls"))
        {
            k_ls();
        }
        else if (!strcmp(cmd, "stat"))
        {
            scanf("%s", a1);
            k_stat(a1);
        }
        else if (!strcmp(cmd, "open"))
        {
            scanf("%s", a1);
            printf("fd = %d\n", k_open(a1));
        }
        else if (!strcmp(cmd, "write"))
        {
            scanf("%d %d %s", &v1, &v2, a1);
            k_write(v1, v2, a1);
        }
        else if (!strcmp(cmd, "read"))
        {
            scanf("%d %d", &v1, &v2);
            k_read(v1, v2);
        }
        else if (!strcmp(cmd, "link"))
        {
            scanf("%s %s", a1, a2);
            k_link(a1, a2);
        }
        else if (!strcmp(cmd, "unlink"))
        {
            scanf("%s", a1);
            k_unlink(a1);
        }
        else if (!strcmp(cmd, "truncate"))
        {
            scanf("%s %d", a1, &v1);
            k_truncate(a1, v1);
        }
        else if (!strcmp(cmd, "seek"))
        {
            scanf("%d %d", &v1, &v2);
            k_seek(v1, v2);
        }
        else if (!strcmp(cmd, "close"))
        {
            scanf("%d", &v1);
            k_close(v1);
        }
    }
    return 0;
}