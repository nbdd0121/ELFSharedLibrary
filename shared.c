#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

void* readFile(const char* name) {
    FILE* file = fopen(name, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) == -1) {
        return NULL;
    }
    long length = ftell(file);
    if (length == -1) {
        return NULL;
    }
    rewind(file);
    char* filecontents = malloc(length + 1);
    if (!filecontents) {
        return NULL;
    }
    if (!fread(filecontents, length, 1, file)) {
        free(filecontents);
        return NULL;
    }
    if (fclose(file) == -1) {
        free(filecontents);
        return NULL;
    }
    filecontents[length] = 0;
    return filecontents;
}

void* alloc_exec(size_t size) {
#ifdef _MSC_VER
    char* ptr = VirtualAlloc(NULL, size + 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    *(size_t*)ptr = size;
    return ptr + 4096;
#else
    return aligned_alloc(size, 4096);
#endif
}

void free_exec(void* ptr) {
#ifdef _MSC_VER
    size_t* oldPtr = (size_t*)((char*)ptr - 4096);
    VirtualFree(oldPtr, *oldPtr, MEM_RELEASE);
#else
    free(ptr);
#endif
}