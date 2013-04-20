objects = bootimg.o unpack_bootimg.o repack_bootimg.o \
		  unpack_ramdisk.o repack_ramdisk.o

bootimg : $(objects)
	cc -o bootimg $(objects)

bootimg.o : bootimg.c bootimg.h
unpack_bootimg.o : unpack_bootimg.c
repack_bootimg.o : repack_bootimg.c
unpack_ramdisk.o : unpack_ramdisk.c
repack_ramdisk.o : repack_ramdisk.c

.PHONY : clean
clean :
	rm -f bootimg $(objects)
