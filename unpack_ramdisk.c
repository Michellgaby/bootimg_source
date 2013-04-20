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

int check_ramdisk_magic(char *magic)
{
    int count;
    for(count = 0;count < 6;count++) {
        if (magic[count] != RAMDISK_MAGIC[count]) {
            printf("Cpio Magic not found!\n");
            exit(0);
        }
    }
    return 0;
}

int unpack_ramdisk_usage(void)
{
    fprintf(stderr,"usage: bootimg --unpack-ramdisk\n"
           "\t[ --directory <directory name> ]\n");
    return 1;
}

//int main(int argc, char **argv)
int unpack_ramdisk(int argc, char **argv)
{
    int swrx;
    int namesize;
    int filesize;
    int total_size;
    int if_slink;
    int if_file;
    char *directory = "initrd";
    char *ramdisk = "ramdisk.gz";
    char *cpio = "ramdisk-new";
    ramdisk_hdr header;
    unsigned char tmp[8];
    unsigned char tmp_path[PATH_MAX];
    unsigned char name[PATH_MAX];
    unsigned char slink_path[PATH_MAX];
    unsigned char GZIP_MAGIC[4] = {0x1f, 0x8b, 0x08, 0x00};
    swrx = namesize = filesize = total_size = if_slink = if_file = 0;

    if(argc == 2) {
        return unpack_ramdisk_usage();
    }

    if(argc > 1) {
        argc--;
        argv++;
    }


    while(argc > 1){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
            return unpack_ramdisk_usage();
        }
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--directory")) {
            directory = val;
        } else {
            return unpack_ramdisk_usage();
        }
    }


    FILE *f;
    int count;
    if ((f=fopen(ramdisk, "r")) == NULL) {
        printf("\"%s\":no such file or directory\n", ramdisk);
        return -1;
    }

    fread(&tmp, 4, 1, f);
    for(count = 0;count < 4;count++) {
        if (tmp[count] != GZIP_MAGIC[count]) {
            if_file = 1;
        }
    }

    if(!if_file) {
        sprintf(tmp_path,"cp %s ramdisk-new.gz",ramdisk);system(tmp_path); // linux
        argv[0] = "minigzip";
        argv[1] = "-d";
        argv[2] = "ramdisk-new.gz";
        argc=3;
        minigzip(argc,argv);
    } else {
        fseek(f,0,SEEK_SET);
        fread(&header, sizeof(header), 1, f);
        for(count = 0;count < 6;count++) {
            if (header.magic[count] != RAMDISK_MAGIC[count]) {
                printf("The %s not gzip or cpio!\n",ramdisk);
                exit(0);
            }
        }
        cpio = ramdisk;
    }

    if_file = 0;
    fclose(f);

    if ((f=fopen(cpio, "r")) == NULL) {
        printf("\"%s\":no such file or directory\n", cpio);
        return -1;
    }

    fread(&header, sizeof(header), 1, f);
    check_ramdisk_magic(header.magic);
    fseek(f,0,SEEK_SET);

    cut();
    printf("ramdisk file: %s\n",ramdisk);
    printf("directory: %s\n", directory);
    printf("outfile: cpiolist.txt\n");
    cut();

    if(!access(directory, 0)) {
        printf("Please remove %s\n",directory);
        remove("ramdisk-new");
        exit(0);
    }

    FILE *cpiolist;
    cpiolist = fopen("cpiolist.txt", "wb");

    mkdir(directory,0777);

    while(!feof(f)) {
        memset(name, 0, sizeof(name));
        memset(tmp_path, 0, sizeof(tmp_path));
        memset(slink_path, 0, sizeof(slink_path));

        while(total_size & 3) {
            total_size++;
            fseek(f, 1, SEEK_CUR);
        }

        fread(&header, sizeof(header), 1, f);
        check_ramdisk_magic(header.magic);
        memcpy(tmp,header.mode,8);
        swrx = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.namesize,8);
        namesize = (unsigned)strtoul(tmp, 0, 16);
        memcpy(tmp,header.filesize,8);
        filesize = (unsigned)strtoul(tmp, 0, 16);
        fread(&name, namesize-1, 1, f);
        fseek(f, 1, SEEK_CUR);

        if(!strcmp(name, "TRAILER!!!"))
            break;

        total_size += 6 + 8*13 + (namesize-1) + 1;

        while(total_size & 3) {
            total_size++;
            fseek(f, 1, SEEK_CUR);
        }

        if (S_ISDIR(swrx)) {
            swrx = swrx & 0777;
            sprintf(tmp_path,"%s/%s",directory,name);
            mkdir(tmp_path,swrx);
            //fprintf(cpiolist, "dir /%s 0%o\n", name, swrx);
            fprintf(cpiolist, "dir %s 0%o\n", name, swrx);
            memset(tmp_path, 0, sizeof(tmp_path));
        } else if (S_ISREG(swrx)) {
            swrx = swrx & 0777;
            //fprintf(cpiolist, "file /%s %s/%s 0%o\n", name, directory, name, swrx);
            fprintf(cpiolist, "file %s %s/%s 0%o\n", name, directory, name, swrx);
            if_file = 1;
        } else if (S_ISLNK(swrx)) {
            swrx = swrx & 0777;
            fread(&slink_path, filesize, 1, f);
            //fprintf(cpiolist, "slink /%s %s 0%o\n", name, slink_path, swrx);
            fprintf(cpiolist, "slink %s %s 0%o\n", name, slink_path, swrx);
            fseek(f,-filesize,SEEK_CUR);
            if_slink = 1;
        }

        if(if_file && !if_slink) {
            sprintf(tmp_path,"%s/%s",directory,name);
            FILE *r = fopen(tmp_path, "wb");
            byte* filedata = (byte*)malloc(filesize);
            fread(filedata, filesize, 1, f);
            fwrite(filedata, filesize, 1, r);
            fclose(r);
            //fseek(f,filesize,SEEK_CUR);
            total_size += filesize;
            if_file = 0;
        } else {
            fseek(f,filesize,SEEK_CUR);
            total_size += filesize;
            if_slink = 0;
        }
    }
    fclose(cpiolist);
    fclose(f);
    remove("ramdisk-new");
}
