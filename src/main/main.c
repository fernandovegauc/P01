#include "../osfs/os_API.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    os_mount("disks/simdiskfilled.bin");
    // os_mount("disks/simdiskformat.bin");
    os_bitmap(1, 1);
    // os_ls("/");
    // printf("\n");
    char* folder = "/";
    os_ls(folder);
    printf("\n");
    // os_ls("/memes");
    // printf("\n");
    // os_ls("/music");
    // printf("\n");
    // os_ls("/memes");
    // os_ls("/music");
    // os_ls("/");
    os_unmount();
    return 0;
}


