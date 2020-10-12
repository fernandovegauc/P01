#include "../osfs/os_API.h"

int main(int argc, char const *argv[])
{
    os_mount("disks/simdiskfilled.bin");
    // os_mount("disks/simdiskformat.bin");
    os_bitmap(0, 1);
    // os_bitmap(1, 1);
    // os_ls("/");
    os_unmount();
    return 0;
}


