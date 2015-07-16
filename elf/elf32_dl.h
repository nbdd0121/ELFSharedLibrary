#ifndef NORLIT_ELF_ELF32_DL_H
#define NORLIT_ELF_ELF32_DL_H

#include <stdint.h>

#define RTLD_LAZY 0
#define RTLD_NOW 1
#define RTLD_GLOBAL 2
#define RTLD_LOCAL 0

void* ELF32_dlopen(const char* name, int flags);
void* ELF32_dlsym(void* handle, const char* name);
void ELF32_dlclose(void* handle);
char* ELF32_dlerror(void);
void ELF32_addGlobalSymbol(const char* name, void* symbol);

#endif