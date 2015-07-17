#ifndef NORLIT_ELF_ELF64_DL_H
#define NORLIT_ELF_ELF64_DL_H

#include <stdint.h>

#define RTLD_LAZY 0
#define RTLD_NOW 1
#define RTLD_GLOBAL 2
#define RTLD_LOCAL 0

void* ELF64_dlopen(const char* name, int flags);
void* ELF64_dlsym(void* handle, const char* name);
void ELF64_dlclose(void* handle);
char* ELF64_dlerror(void);
void ELF64_addGlobalSymbol(const char* name, void* symbol);

#endif