#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include "bootimg.h"

#define SHA_DIGEST_SIZE 20
#define rol(bits, value) (((value) << (bits)) | ((value) >> (32 - (bits))))

typedef struct SHA_CTX {
    uint64_t count;
    uint32_t state[5];
#if defined(HAVE_ENDIAN_H) && defined(HAVE_LITTLE_ENDIAN)
    union {
        uint8_t b[64];
        uint32_t w[16];
    } buf;
#else
    uint8_t buf[64];
#endif
} SHA_CTX;

static void SHA1_transform(SHA_CTX *ctx) {
    uint32_t W[80];
    uint32_t A, B, C, D, E;
    uint8_t *p = ctx->buf;
    int t;

    for(t = 0; t < 16; ++t) {
        uint32_t tmp =  *p++ << 24;
        tmp |= *p++ << 16;
        tmp |= *p++ << 8;
        tmp |= *p++;
        W[t] = tmp;
    }

    for(; t < 80; t++) {
        W[t] = rol(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];

    for(t = 0; t < 80; t++) {
        uint32_t tmp = rol(5,A) + E + W[t];

        if (t < 20)
            tmp += (D^(B&(C^D))) + 0x5A827999;
        else if ( t < 40)
            tmp += (B^C^D) + 0x6ED9EBA1;
        else if ( t < 60)
            tmp += ((B&C)|(D&(B|C))) + 0x8F1BBCDC;
        else
            tmp += (B^C^D) + 0xCA62C1D6;

        E = D;
        D = C;
        C = rol(30,B);
        B = A;
        A = tmp;
    }

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
}

void SHA_update(SHA_CTX *ctx, const void *data, int len) {
    int i = ctx->count % sizeof(ctx->buf);
    const uint8_t* p = (const uint8_t*)data;

    ctx->count += len;

    while (len--) {
        ctx->buf[i++] = *p++;
        if (i == sizeof(ctx->buf)) {
            SHA1_transform(ctx);
            i = 0;
        }
    }
}

uint8_t *SHA_final(SHA_CTX *ctx) {
    uint8_t *p = ctx->buf;
    uint64_t cnt = ctx->count * 8;
    int i;

    SHA_update(ctx, (uint8_t*)"\x80", 1);
    while ((ctx->count % sizeof(ctx->buf)) != (sizeof(ctx->buf) - 8)) {
        SHA_update(ctx, (uint8_t*)"\0", 1);
    }
    for (i = 0; i < 8; ++i) {
        uint8_t tmp = cnt >> ((7 - i) * 8);
        SHA_update(ctx, &tmp, 1);
    }

    for (i = 0; i < 5; i++) {
        uint32_t tmp = ctx->state[i];
        *p++ = tmp >> 24;
        *p++ = tmp >> 16;
        *p++ = tmp >> 8;
        *p++ = tmp >> 0;
    }

    return ctx->buf;
}

void SHA_init(SHA_CTX* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

int repack_bootimg_usage(void)
{
    fprintf(stderr,"usage: bootimg --repack-bootimg\n"
            "\t--kernel <filename>\n"
            "\t--ramdisk <filename>\n"
            "\t[ --second <2ndbootloader-filename> ]\n"
            "\t[ --cmdline <kernel-commandline> ]\n"
            "\t[ --board <boardname> ]\n"
            "\t[ --base <address> ]\n"
            "\t[ --pagesize <pagesize> ]\n"
            "\t[ --ramdisk_offset <address> ]\n"
            "\t-o|--output <filename>\n"
            );
    return 1;
}



static unsigned char padding[131072] = { 0, };

int write_padding(int fd, unsigned pagesize, unsigned itemsize)
{
    unsigned pagemask = pagesize - 1;
    unsigned count;

    if((itemsize & pagemask) == 0) {
        return 0;
    }

    count = pagesize - (itemsize & pagemask);

    if(write(fd, padding, count) != count) {
        return -1;
    } else {
        return 0;
    }
}

int repack_bootimg(int argc, char **argv)
{
    bootimg_hdr hdr;

    char *kernel_fn = 0;
    void *kernel_data = 0;
    char *ramdisk_fn = 0;
    void *ramdisk_data = 0;
    char *second_fn = 0;
    void *second_data = 0;
    char *cmdline = "";
    char *bootimg = 0;
    char *board = "";
    unsigned pagesize = 2048;
    int fd;
    SHA_CTX ctx;
    uint8_t* sha;
    unsigned base           = 0x10000000;
    unsigned kernel_offset  = 0x00008000;
    unsigned ramdisk_offset = 0x01000000;
    unsigned second_offset  = 0x00f00000;
    unsigned tags_offset    = 0x00000100;
    argc--;
    argv++;

    memset(&hdr, 0, sizeof(hdr));

    if (!access("bootinfo.txt", 0))
    {
        FILE* BOOTINFO = fopen("bootinfo.txt", "r");
        if (BOOTINFO == NULL)
        {
            fprintf(stderr, "error: could not create '%s'\n", "bootinfo.txt");
            return -1;
        }

        while(!feof(BOOTINFO))
        {
            unsigned char tmp[512 + 1];
            unsigned char info_magic[14];
            unsigned char info_board[16];
            unsigned char info_cmdline[513];

            fgets(tmp, 18, BOOTINFO);

            memcpy(info_magic, tmp, 14);
            memset(tmp, 0, sizeof(tmp));

            if(!strcmp(info_magic, "Kernel  [addr]")) {
                fgets(tmp, 10 + 1, BOOTINFO);
                hdr.kernel_addr = strtoul(tmp, 0, 16);
            } else if(!strcmp(info_magic, "Ramdisk [addr]")) {
                fgets(tmp, 10 + 1, BOOTINFO);
                hdr.ramdisk_addr = strtoul(tmp, 0, 16);
            } else if(!strcmp(info_magic, "Second  [addr]")) {
                fgets(tmp, 10 + 1, BOOTINFO);
                hdr.second_addr = strtoul(tmp, 0, 16);
            } else if(!strcmp(info_magic, "Tags    [addr]")) {
                fgets(tmp, 10 + 1, BOOTINFO);
                hdr.tags_addr = strtoul(tmp, 0, 16);
            } else if(!strcmp(info_magic, "Board   [char]")) {
                memset(info_board, 0, sizeof(info_board));
                fgets(tmp, 16 + 1, BOOTINFO);
                memcpy(info_board, tmp, strlen(tmp) - 1);
                board = info_board;
            } else if(!strcmp(info_magic, "Cmdline [char]")) {
                memset(info_cmdline, 0, sizeof(info_cmdline));
                fgets(tmp, 513 + 1, BOOTINFO);
                memcpy(info_cmdline, tmp, strlen(tmp) - 1);
                cmdline = info_cmdline;
            } else if(!strcmp(info_magic, "Page    [size]")) {
                fgets(tmp, 10 + 1, BOOTINFO);
                pagesize = strtoul(tmp, 0, 10);
                if ((pagesize != 2048) && (pagesize != 4096) && (pagesize != 8192) && (pagesize != 16384) && (pagesize != 32768) && (pagesize != 65536) && (pagesize != 131072)) {
                fprintf(stderr,"error: unsupported page size %d\n", pagesize);
                return -1;
                }
            }
        }
        fclose(BOOTINFO);

        bootimg = "boot.img";
        kernel_fn = "kernel";
        if (!access("ramdisk-new.gz", 0))
            ramdisk_fn = "ramdisk-new.gz";
        else
            ramdisk_fn = "ramdisk.gz";

    }

    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
            return repack_bootimg_usage();
        }
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            bootimg = val;
        } else if(!strcmp(arg, "--kernel")) {
            kernel_fn = val;
        } else if(!strcmp(arg, "--ramdisk")) {
            ramdisk_fn = val;
        } else if(!strcmp(arg, "--second")) {
            second_fn = val;
        } else if(!strcmp(arg, "--cmdline")) {
            cmdline = val;
        } else if(!strcmp(arg, "--base")) {
            base = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--kernel_offset")) {
            kernel_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--ramdisk_offset")) {
            ramdisk_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--second_offset")) {
            second_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--tags_offset")) {
            tags_offset = strtoul(val, 0, 16);
        } else if(!strcmp(arg, "--board")) {
            board = val;
        } else if(!strcmp(arg,"--pagesize")) {
            pagesize = strtoul(val, 0, 10);
            if ((pagesize != 2048) && (pagesize != 4096) && (pagesize != 8192) && (pagesize != 16384) && (pagesize != 32768) && (pagesize != 65536) && (pagesize != 131072)) {
                fprintf(stderr,"error: unsupported page size %d\n", pagesize);
                return -1;
            }
        } else {
            return repack_bootimg_usage();
        }
    }
    hdr.page_size = pagesize;

    cut();
    fprintf(stderr, "kernel  : 0x%08x\n"
                    "Ramdisk : 0x%08x\n"
                    "Secound : 0x%08x\n"
                    "Tags    : 0x%08x\n"
                    "Page    : %d\n"
                    "Board   : %s\n"
                    "Cmdline : %s\n"
                    "Outfile : %s\n"
                    ,hdr.kernel_addr
                    ,hdr.ramdisk_addr
                    ,hdr.second_addr
                    ,hdr.tags_addr
                    ,pagesize
                    ,board
                    ,cmdline
                    ,bootimg);
    cut();

    if(bootimg == 0) {
        fprintf(stderr,"error: no output filename specified\n");
        return repack_bootimg_usage();
    }

    if(kernel_fn == 0) {
        fprintf(stderr,"error: no kernel image specified\n");
        return repack_bootimg_usage();
    }

    if(ramdisk_fn == 0) {
        fprintf(stderr,"error: no ramdisk image specified\n");
        return repack_bootimg_usage();
    }

    if(strlen(board) >= BOOT_NAME_SIZE) {
        fprintf(stderr,"error: board name too large\n");
        return repack_bootimg_usage();
    }

    strcpy(hdr.name, board);

    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    if(strlen(cmdline) > (BOOT_ARGS_SIZE - 1)) {
        fprintf(stderr,"error: kernel commandline too large\n");
        return 1;
    }
    strcpy((char*)hdr.cmdline, cmdline);

    kernel_data = load_file(kernel_fn, &hdr.kernel_size);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        return 1;
    }

    if(!strcmp(ramdisk_fn,"NONE")) {
        ramdisk_data = 0;
        hdr.ramdisk_size = 0;
    } else {
        ramdisk_data = load_file(ramdisk_fn, &hdr.ramdisk_size);
        if(ramdisk_data == 0) {
            fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
            return 1;
        }
    }

    if(second_fn) {
        second_data = load_file(second_fn, &hdr.second_size);
        if(second_data == 0) {
            fprintf(stderr,"error: could not load secondstage '%s'\n", second_fn);
            return 1;
        }
    }

    /* put a hash of the contents in the header so boot images can be
     * differentiated based on their first 2k.
     */
    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, hdr.kernel_size);
    SHA_update(&ctx, &hdr.kernel_size, sizeof(hdr.kernel_size));
    SHA_update(&ctx, ramdisk_data, hdr.ramdisk_size);
    SHA_update(&ctx, &hdr.ramdisk_size, sizeof(hdr.ramdisk_size));
    SHA_update(&ctx, second_data, hdr.second_size);
    SHA_update(&ctx, &hdr.second_size, sizeof(hdr.second_size));
    sha = SHA_final(&ctx);
    memcpy(hdr.id, sha,
           SHA_DIGEST_SIZE > sizeof(hdr.id) ? sizeof(hdr.id) : SHA_DIGEST_SIZE);

    fd = open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if(fd < 0) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        return 1;
    }

    if(write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) goto fail;
    if(write_padding(fd, pagesize, sizeof(hdr))) goto fail;

    if(write(fd, kernel_data, hdr.kernel_size) != hdr.kernel_size) goto fail;
    if(write_padding(fd, pagesize, hdr.kernel_size)) goto fail;

    if(write(fd, ramdisk_data, hdr.ramdisk_size) != hdr.ramdisk_size) goto fail;
    if(write_padding(fd, pagesize, hdr.ramdisk_size)) goto fail;

    if(second_data) {
        if(write(fd, second_data, hdr.second_size) != hdr.second_size) goto fail;
        if(write_padding(fd, pagesize, hdr.ramdisk_size)) goto fail;
    }

    return 0;

fail:
    unlink(bootimg);
    close(fd);
    fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
            strerror(errno));
    return 1;
}
