#include "../osfs/os_API.h"
#include <stdio.h>

void print_block(unsigned int, int);
int main(int argc, char const *argv[])
{
    // os_mount("disks/simdiskfilled.bin");
    // os_mount("disks/simdiskformat.bin");
    os_mount("disks/simdiskformat(copy2).bin");
    // os_bitmap(1,1);
    // print_block(0,0);
    // osFile* file = os_open("/archivo.txt", 'w');
    // os_ls("/");
    // os_unload("/", "new_root");
    // os_load("memes");
    // os_close(file);
    os_ls("/");
    os_hardlink("generic (2).jpg", "folder1/meme1.jpg");
    os_unload("/", "new_root");
    // os_unload("/IMPORTANT.txt", "important.txt");
    // os_unload("/memes", "memes");
    // printf("Existe /memes: %d\n", os_exists("/memes"));
    // printf("Existe /folder/nope: %d\n", os_exists("/foler/nope"));
    // printf("Existe /IMPORTANT.txt: %d\n", os_exists("/IMPORTANT.txt"));
    // printf("Existe /folder/god: %d\n", os_exists("/folder/god"));
    // os_unload("/", "root");
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


