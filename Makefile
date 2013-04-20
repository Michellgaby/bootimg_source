bootimg :
	gcc -o bootimg bootimg.c unpack_bootimg.c repack_bootimg.c unpack_ramdisk.c \
             repack_ramdisk.c zlib/test/minigzip.c zlib/adler32.c \
             zlib/compress.c zlib/crc32.c zlib/deflate.c \
             zlib/gzclose.c zlib/gzlib.c zlib/gzread.c \
             zlib/gzwrite.c zlib/infback.c zlib/inflate.c \
             zlib/inftrees.c zlib/inffast.c zlib/slhash.c  \
             zlib/trees.c zlib/uncompr.c zlib/zutil.c

.PHONY : clean
clean :
	rm -f bootimg
