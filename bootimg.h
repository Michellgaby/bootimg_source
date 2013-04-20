#ifndef _BOOT_IMAGE_H
#define _BOOT_IMAGE_H

/*==========================================================================================
 * bootimg function
 */

int repack_bootimg(int argc, char **argv);
int repack_ramdisk(int argc, char **argv);
int unpack_bootimg(int argc, char **argv);
int unpack_ramdisk(int argc, char **argv);

/*==========================================================================================
 * bootimg hdr
 */

typedef struct bootimg_hdr bootimg_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

struct bootimg_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;
    unsigned kernel_addr;

    unsigned ramdisk_size;
    unsigned ramdisk_addr;

    unsigned second_size;
    unsigned second_addr;

    unsigned tags_addr;
    unsigned page_size;
    unsigned unused[2];

    unsigned char name[BOOT_NAME_SIZE];

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8];
};

/*==========================================================================================
 * ramdisk hdr
 */

typedef struct ramdisk_hdr ramdisk_hdr;

#define RAMDISK_MAGIC "070701"

struct ramdisk_hdr
{
    unsigned char magic[6];
    unsigned char ino[8];
    unsigned char mode[8];
    unsigned char uid[8];
    unsigned char gid[8];
    unsigned char nlink[8];
    unsigned char mtime[8];
    unsigned char filesize[8];
    unsigned char dev_maj[8];
    unsigned char dev_min[8];
    unsigned char rdev_maj[8];
    unsigned char rdev_min[8];
    unsigned char namesize[8];
    unsigned char chksum[8];
};

#endif
