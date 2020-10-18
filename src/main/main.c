#include "../osfs/os_API.h"
#include <stdio.h>

void remove_file(char* path);
void update_bitmap(unsigned int block, int value);
unsigned int free_space();

void print_block(unsigned int, int);
int main(int argc, char const *argv[])
{
    // os_mount("disks/simdiskfilled(copy).bin");
    os_mount("disks/simdiskformat(copy).bin");
    // os_mount("disks/simdiskformat(copy3).bin");
    // os_rmdir("/", 1);
    // os_bitmap(1,1);
    // os_rmdir("/", 1);
    os_ls("/");
    os_load("root");
    // osFile* file = os_open("/memes/hola.txt", 'w');
    // char s[100] = "holaaaaa\n";
    // os_write(file, s, 100);
    // os_close(file);
    // os_unload("/memes/hola.txt", "hola.txt");
    
    os_unmount();
    return 0;
}


