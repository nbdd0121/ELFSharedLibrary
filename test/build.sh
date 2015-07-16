i686-elf-gcc -fPIC -c dynamic.c
i686-elf-gcc -fPIC -c main.c
i686-elf-ld -shared dynamic.o -o libdynamic.so
i686-elf-ld -shared main.o -L . -ldynamic -o libmain.so
i686-elf-strip libdynamic.so libmain.so