#include "../osfs/os_API.h"
#include <stdio.h>

void remove_file(char* path);
void update_bitmap(unsigned int block, int value);
unsigned int free_space();

void print_block(unsigned int, int);
int main(int argc, char const *argv[])
{
    os_mount("disks/simdiskfilled(copy).bin");
    // os_mount("disks/simdiskformat(copy).bin");
    // os_mount("disks/simdiskformat(copy3).bin");
    // os_rmdir("/", 1);
    // os_load("memes");
    printf("folder %d\n", os_exists("/folder"));
    printf("folder/god %d\n", os_exists("/folder/god"));
    printf("memes/generic (1).jpg %d\n", os_exists("/memes/generic (1).jpg"));
    printf("fold %d\n", os_exists("/fold"));
    os_ls("/");
    printf("freespace %u\n", free_space());

    os_unmount();
    return 0;
}


