#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

int cut()
{
    struct winsize size;  
    int input_fd = 0;
    int i;
  
    if (ioctl (input_fd, TIOCGWINSZ, &size) == -1)  
        printf("");
    else
        for(i = 0;i < size.ws_col;i++)
            printf("-");

    printf("\n");
}

int usage()
{
    fprintf(stderr, "usage: bootimg\n"
            "\t--unpack-bootimg [--help]\n"
            "\t--unpack-ramdisk [--help]\n"
            "\t--repack-bootimg [--help]\n"
            "\t--repack-ramdisk [--help]\n"
            );
    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return usage();

    argc--;
    argv++;

    if (!strcmp(argv[0], "--unpack-bootimg"))
        unpack_bootimg(argc, argv);
    else if (!strcmp(argv[0], "--unpack-ramdisk"))
        unpack_ramdisk(argc, argv);
    else if (!strcmp(argv[0], "--repack-bootimg"))
        repack_bootimg(argc, argv);
    else if (!strcmp(argv[0], "--repack-ramdisk"))
        repack_ramdisk(argc, argv);
    else return usage();
}
