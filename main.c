#include <stdio.h>
#include <stdlib.h>
#include <elf/elf32_dl.h>
#include <direct.h>

#include <math.h>
#include <Windows.h>

void* mmap(void* p1, int size, int i3, int i4, int i5, int i6) {
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void munmap(void* ptr, int size) {
    VirtualFree(ptr, size, MEM_RELEASE);
}

int main(void) {
    _chdir("test");
    const char* filename = "libmain.so";

    ELF32_addGlobalSymbol("puts", puts);
    ELF32_addGlobalSymbol("printf", printf);
    ELF32_addGlobalSymbol("abort", abort);

    ELF32_addGlobalSymbol("mmap", mmap);
    ELF32_addGlobalSymbol("munmap", munmap);

    void* handle = ELF32_dlopen(filename, RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "Cannot open %s: %s\n", filename, ELF32_dlerror());
        return 1;
    }

    int(*main)(double) = ELF32_dlsym(handle, "main");
    if (main) {
        printf("RET: %d\n", main(NAN));
    }

    ELF32_dlclose(handle);
    return 0;
}