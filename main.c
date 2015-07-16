#include <stdio.h>
#include <elf/elf32_dl.h>
#include <direct.h>

int main(void) {
    _chdir("test");
    const char* filename = "libdynamic.so";

    ELF32_addGlobalSymbol("puts", puts);

    void* handle = ELF32_dlopen(filename, RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "Cannot open %s: %s\n", filename, ELF32_dlerror());
        return 1;
    }

    int(*createString)(void) = ELF32_dlsym(handle, "createString");

    printf("%p\n", createString);

    for (int i = 0; i < 10; i++) {
        printf("%s\n", createString());
    }

    ELF32_dlclose(handle);
    return 0;
}