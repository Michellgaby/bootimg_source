objects = bootimg.o unpack_bootimg.o repack_bootimg.o

bootimg : $(objects)
	cc -o bootimg $(objects)

bootimg.o : bootimg.c bootimg.h
unpack_bootimg.o : unpack_bootimg.c bootimg.h
repack_bootimg.o : repack_bootimg.c bootimg.h

.PHONY : clean
clean :
	rm -f bootimg $(objects)
