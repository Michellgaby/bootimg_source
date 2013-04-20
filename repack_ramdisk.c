#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdarg.h>
#include <fcntl.h>

#include "bootimg.h"

typedef unsigned char byte;
static int total_size = 0;

int repack_ramdisk_usage(void)
{
    fprintf(stderr,"usage: bootimg --repack-ramdisk\n"
           "\t[ -l|--list <filename> ]\n");
    return 1;
}

int write_data(char *infile, int type)
{
    struct stat size_tmp;
    int datasize;
    FILE *stream;

    stream = fopen("ramdisk-new", "a-");

    if(type) {
        fwrite(infile, sizeof(infile), 1, stream);
        total_size += sizeof(infile);
        fclose(stream);
        return 0;
    }

    FILE *f;
    if ((f = fopen(infile, "r")) == NULL) {
        printf("\"%s\":no such file or directory\n", infile);
        return -1;
    }

    stat(infile, &size_tmp);
    datasize = (int)size_tmp.st_size;
    if(datasize) {
        byte* data = (byte*)malloc(datasize);
        fread(data, datasize, 1, f);
        fwrite(data, datasize, 1, stream);
        total_size += datasize;
    }

    fclose(f);
    fclose(stream);
    return 0;
}

int write_cpio_header(char *output, int mode, int filesize)
{
    int namesize = strlen(output) + 1;
    static unsigned next_inode = 300000;

    FILE *stream;
    stream = fopen("ramdisk-new", "a-");

    while(total_size & 3) {
        total_size++;
        fputc(0, stream);
    }

    fprintf(stream,"%06x%08x%08x%08x%08x%08x%08x"
           "%08x%08x%08x%08x%08x%08x%08x%s%c",
           0x070701,
           next_inode++,  //  s.st_ino,
           mode,
           0, // s.st_uid,
           0, // s.st_gid,
           1, // s.st_nlink,
           0, // s.st_mtime,
           filesize,
           0, // volmajor
           0, // volminor
           0, // devmajor
           0, // devminor,
           namesize,
           0,
           output,
           0
           );

    total_size += 6 + 8*13 + strlen(output) + 1;

    while(total_size & 3) {
        total_size++;
        fputc(0, stream);
    }
    fclose(stream);
    return 0;
}

//int main(int argc, char *argv[])
int repack_ramdisk(int argc, char *argv[])
{
    struct stat size_tmp;
    int swrx;
    char file_type[PATH_MAX];
    char file_name[PATH_MAX];
    char file_path[PATH_MAX];
    char mode[PATH_MAX];
    char *cpiolist;
    char *ramdisk = "ramdisk-new.gz";
    FILE *listfile;
    FILE *filename;

    if(argc == 1) {
        cpiolist = "cpiolist.txt";
    } else if(!strcmp(argv[1], "--list") || !strcmp(argv[1], "-l")) {
        if(argc > 2) cpiolist = argv[2];
        else die("--list argument not found");
    } else {
        return repack_ramdisk_usage();
    }

    cut();
    printf("cpiolist file: %s\n",cpiolist);
    printf("outfile: %s\n", ramdisk);
    cut();

    if(!access(cpiolist, 0))
        remove("ramdisk-new");
    else
        die("%s no such file",cpiolist);

    listfile=fopen(cpiolist, "r");
    while(!feof(listfile)) {
        fscanf(listfile, "%s", file_type);;
        if(!strcmp(file_type, "dir")) {
            fscanf(listfile, "%s %s", file_name, mode);
            swrx = strtoul(mode, 0, 8) | S_IFDIR;
            write_cpio_header(file_name, swrx, 0);
        } else if(!strcmp(file_type, "file")) {
            fscanf(listfile, "%s %s %s", file_name, file_path, mode);
            swrx = strtoul(mode, 0, 8) | S_IFREG;
            if ((filename = fopen(file_path, "r")) == NULL) {
                printf("\"%s\":no such file or directory\n", file_path);
                return -1;
            }
            stat(file_path, &size_tmp);
            write_cpio_header(file_name, swrx, (int)size_tmp.st_size);
            fclose(filename);
            write_data(file_path, 0);
        } else if(!strcmp(file_type, "slink")) {
            fscanf(listfile, "%s %s %s", file_name, file_path, mode);
            swrx = strtoul(mode, 0, 8) | S_IFLNK;
            write_cpio_header(file_name, swrx, strlen(file_path));
            write_data(file_path, 1);
        } else if(!strcmp(file_type, "nod")) {
            printf("nod is not implemented\n");
        } else if(!strcmp(file_type, "pipe")) {
            printf("pipe is not implemented\n");
        } else if(!strcmp(file_type, "sock")) {
            printf("sock is not implemented\n");
        }

        memset(mode, 0, sizeof(mode));
        memset(file_type, 0, sizeof(file_type));
        memset(file_name, 0, sizeof(file_name));
        memset(file_path, 0, sizeof(file_path));
    }

    FILE *stream;
    stream = fopen("ramdisk-new", "a-");
    write_cpio_header("TRAILER!!!", 0644, 0);
    while(total_size & 0xff) {
        total_size++;
        fputc(0, stream);
    }

    fclose(stream);
    fclose(listfile);

    argv[0] = "minigzip";
    argv[1] = "ramdisk-new";
    argc=2;
    minigzip(argc,argv);

    return 0;
}
