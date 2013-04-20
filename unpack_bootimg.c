#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include "bootimg.h"

typedef unsigned char byte;

int read_padding(FILE* f, unsigned itemsize, int pagesize)
{
    byte* buf = (byte*)malloc(sizeof(byte) * pagesize);
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        free(buf);
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    fread(buf, count, 1, f);
    free(buf);
    return count;
}

void write_string_to_file(char* file, char* string)
{
    FILE* f = fopen(file, "w");
    fwrite(string, strlen(string), 1, f);
    fclose(f);
}

int unpack_bootimg_usage(void)
{
    fprintf(stderr,"usage: bootimg \n"
           "\t--unpack-bootimg <filename>\n");
    return 1;
}

int unpack_bootimg(int argc, char** argv)
{
    struct stat size_tmp;
    int pagesize = 0;
    int check_magic = 0;
    char tmp[PATH_MAX];
    char* directory = "./";
    char* filename = "boot.img";
    unsigned char boot_magic[8];

    argc--;
    argv++;

    if (argc > 0)
    {
        if (!strcmp(argv[0], "--help"))
            return unpack_bootimg_usage();

        filename = argv[0];
    }

    int total_read = 0;
    FILE *f;
    if ((f = fopen(filename, "rb")) == NULL) {
        printf("\"%s\":no such file or directory\n", filename);
        return -1;
    }

    stat(filename, &size_tmp);
    if(2048 > (int)size_tmp.st_size)
        return unpack_bootimg_usage();

    fseek(f,0,SEEK_SET);
    fread(&boot_magic, sizeof(boot_magic), 1, f);
    if (strcmp(boot_magic, BOOT_MAGIC))
    {
        printf("Android Magic not found!\n");
        return -1;
    }

    fseek(f,-8,SEEK_CUR);
    bootimg_hdr header;
    fread(&header, sizeof(header), 1, f);

    cut();
    printf("Kernel  |addr| = 0x%08x\n" , header.kernel_addr  /*- 0x00008000*/);
    printf("Ramdisk |addr| = 0x%08x\n" , header.ramdisk_addr /*- 0x01000000*/);
    printf("Second  |addr| = 0x%08x\n" , header.second_addr  /*- 0x00f00000*/);
    printf("Tags    |addr| = 0x%08x\n" , header.tags_addr    /*- 0x00000100*/);
    printf("Page    |size| = \"%d\"\n" , header.page_size);
    printf("Board   |char| = \"%s\"\n" , header.name);
    printf("Cmdline |char| = \"%s\"\n" , header.cmdline);
    cut();

    pagesize = header.page_size;
    
    char boottmp[600];
    sprintf(boottmp, "Kernel  [addr] = 0x%08x\n"
                     "Ramdisk [addr] = 0x%08x\n"
                     "Second  [addr] = 0x%08x\n"
                     "Tags    [addr] = 0x%08x\n"
                     "Page    [size] = %d\n"
                     "Board   [char] = %s\n"
                     "Cmdline [char] = %s\n",
    header.kernel_addr, header.ramdisk_addr, header.second_addr,
    header.tags_addr, header.page_size, header.name, header.cmdline);
    write_string_to_file("bootinfo.txt", boottmp);

    total_read += sizeof(header);
    total_read += read_padding(f, sizeof(header), pagesize);

    FILE *k = fopen("kernel","wb");
    byte* kernel = (byte*)malloc(header.kernel_size);

    fread(kernel, header.kernel_size, 1, f);
    total_read += header.kernel_size;
    fwrite(kernel, header.kernel_size, 1, k);
    fclose(k);

    total_read += read_padding(f, header.kernel_size, pagesize);

    FILE *r = fopen("ramdisk.gz", "wb");
    byte* ramdisk = (byte*)malloc(header.ramdisk_size);

    fread(ramdisk, header.ramdisk_size, 1, f);
    total_read += header.ramdisk_size;
    fwrite(ramdisk, header.ramdisk_size, 1, r);
    fclose(r);
    
    fclose(f);

    return 0;
}
