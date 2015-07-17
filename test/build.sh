i686-elf-gcc -fPIC -c dynamic.c
i686-elf-gcc -fPIC -c main.c
i686-elf-ld -shared dynamic.o -o libdynamic.so
i686-elf-ld -shared main.o -L . -ldynamic -o libmain.so
i686-elf-strip libdynamic.so libmain.so

x86_64-elf-gcc -fPIC -c dynamic.c -o dynamic64.o
x86_64-elf-gcc -fPIC -c main.c -o main64.o
x86_64-elf-ld -shared dynamic64.o -o libdynamic64.so
x86_64-elf-ld -shared main64.o -L . -ldynamic64 -o libmain64.so
x86_64-elf-strip libdynamic64.so libmain64.so