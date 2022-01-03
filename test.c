#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main() {

    int dev = open("/dev/mydevice", O_RDONLY);
    if (dev == -1)
    {
        printf("Could not read file\n");
        return -1;
    }

    printf("Opening was successful\n");
    close(dev);
    return 0;
}