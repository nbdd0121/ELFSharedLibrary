#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <elf/elf64.h>
#include <elf/elf64_dl.h>
#include <util/list.h>
#include <util/hashmap.h>

typedef struct dl_handle_t  {
    char* name;
    char* strtab;
    hashmap_t* map;
    char* executable;
    void(*fini)(void);

    size_t depDlLen;
    struct dl_handle_t** depDl;

    int refCount;
    bool resolved;

    list_t globalList;
} dl_handle_t;

static const char* errmsg = NULL;
static list_t globalHandle = {&globalHandle, &globalHandle};

void* readFile(const char* name);
void* alloc_exec(size_t size);
void free_exec(void* ptr);

static hashmap_t* getDlMap() {
    static hashmap_t* map = NULL;
    if (!map) {
        map = hashmap_new_string(1);
    }
    return map;
}

static hashmap_t* getGlobalMap(bool init) {
    static hashmap_t* map = NULL;
    if (!map && init) {
        map = hashmap_new_string(1);
    }
    return map;
}

static int ELF64_validate(Elf64_Ehdr* header) {
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
            header->e_ident[EI_MAG1] != ELFMAG1 ||
            header->e_ident[EI_MAG2] != ELFMAG2 ||
            header->e_ident[EI_MAG3] != ELFMAG3) {
        return 0;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64) {
        return 0;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        return 0;
    }

    if (header->e_ident[EI_VERSION] != 1) {
        return 0;
    }

    if (header->e_type != ET_DYN) {
        return 0;
    }

    return 1;
}

static void ELF64_findBounds(Elf64_Ehdr* header, uint64_t* loPtr, uint64_t* hiPtr) {
    uint64_t lo = 0xFFFFFFFF, hi = 0;
    for (int i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr *program = ELF64_PH_GET(header, i);
        if (program->p_type == PT_LOAD) {
            uint64_t seglo = program->p_vaddr;
            uint64_t seghi = seglo + program->p_memsz;

            seglo = seglo / program->p_align * program->p_align;
            seghi = ((seghi - 1) / program->p_align + 1)* program->p_align;

            if (seglo < lo) lo = seglo;
            if (seghi > hi) hi = seghi;
        }
    }
    if (loPtr)
        *loPtr = lo;
    if (hiPtr)
        *hiPtr = hi;
}

static void ELF64_loadProgram(Elf64_Ehdr *header, char* mem) {
    for (int i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr *h = ELF64_PH_GET(header, i);
        if (h->p_type == PT_LOAD) {
            memset(mem + h->p_vaddr + h->p_filesz, 0, (size_t)(h->p_memsz - h->p_filesz));
            memcpy(mem + h->p_vaddr, ELF64_PH_CONTENT(header, h), (size_t)h->p_filesz);
        }
    }
}

static int ELF64_findProgram(Elf64_Ehdr* header, int startIndex, int targetType) {
    for (int i = startIndex; i < header->e_phnum; i++) {
        Elf64_Phdr *section = ELF64_PH_GET(header, i);
        if (section->p_type == targetType) {
            return i;
        }
    }
    return -1;
}

static void* ELF64_resolveSymbolGlobal(const char* name) {
    hashmap_t* map = getGlobalMap(false);
    if (map) {
        void* ret = hashmap_get(map, name);
        if (ret) return ret;
    }
    dl_handle_t* handle;
    list_forEach(&globalHandle, handle, dl_handle_t, globalList) {
        void* ret = ELF64_dlsym(handle, name);
        if (ret) return ret;
    }
    return NULL;
}

static bool ELF64_resolveSymbols(dl_handle_t* handle, char* strtab, char *symtab, int size, uint64_t syment) {
    /* First element is skipeed */
    for (int i = 1; i < size; i++) {
        Elf64_Sym *symbol = (Elf64_Sym *)(symtab + i * syment);

        if (symbol->st_shndx == SHN_UNDEF) {
            // Undefined symbols, we need to resolve from the dependency list

            // Get the name of the symbol
            char *name = strtab + symbol->st_name;

            void* result = ELF64_resolveSymbolGlobal(name);
            for (size_t i = 0; !result && i < handle->depDlLen; i++) {
                result = ELF64_dlsym(handle->depDl[i], name);
            }

            // It is a error if we cannot resolve a strong symbol
            if (!result) {
                if (!(ELF64_ST_BIND(symbol->st_info) & STB_WEAK)) {
                    errmsg = "Unresolved symbol";
                    // printf("[ERROR] [ELF] Failed to resolve %s\n", name);
                    return false;
                }
            }

            symbol->st_shndx = SHN_ABS;
            symbol->st_value = (uint64_t)result;
        } else if (symbol->st_shndx < SHN_LORESERVE) {
            symbol->st_shndx = SHN_ABS;
            symbol->st_value = (uint64_t)(symbol->st_value + handle->executable);
        } else if (symbol->st_shndx != SHN_ABS) {
            errmsg = "Unimplemented st_shndx";
            return false;
        }

        if (ELF64_ST_BIND(symbol->st_info) & STB_GLOBAL) {
            char *name = strtab + symbol->st_name;
            hashmap_put(handle->map, name, (void*)symbol->st_value);
        }
    }

    return true;
}

static bool ELF64_relocateRela(dl_handle_t* handle, char* reltab, uint64_t entsize, uint64_t limit, char* symtab, uint64_t syment) {
    for (char* end = reltab + limit; reltab<end; reltab += entsize) {
        Elf64_Rela* rel = (Elf64_Rela*)reltab;
        Elf64_Sym *symbol = (Elf64_Sym *)(symtab + ELF64_R_SYM(rel->r_info) * syment);
        uint64_t *ref = (uint64_t *)(handle->executable + rel->r_offset);
        switch (ELF64_R_TYPE(rel->r_info)) {
            case R_X86_64_GLOB_DAT:
                *ref = symbol->st_value;
                break;
            case R_X86_64_JUMP_SLOT:
                *ref = symbol->st_value;
                break;
            case R_X86_64_RELATIVE:
                *ref = rel->r_addend + (int)handle->executable;
                break;
            default:
                errmsg = "Unimplemented relocation type";
                printf("Unknown relocation type %d\n", ELF64_R_TYPE(rel->r_info));
                return false;
        }
    }
    return true;
}

static void elf64_dlopen_doit(dl_handle_t* handle, Elf64_Ehdr* header, void(**initptr)(void)) {
    // Check header
    if (!ELF64_validate(header)) {
        errmsg = "Broken shared library";
        return;
    }

    // Allocate executable memory
    uint64_t size;
    ELF64_findBounds(header, NULL, &size);
    handle->executable = alloc_exec((size_t)size);
    if (!handle->executable) {
        errmsg = "Memory allocation failure";
        return;
    }

    // Load binary image into memory
    ELF64_loadProgram(header, handle->executable);

    // Find DYNAMIC section. This is mandatory
    int dynamicSection = ELF64_findProgram(header, 0, PT_DYNAMIC);
    if (dynamicSection == -1) {
        errmsg = "Broken shared library";
        return;
    }

    char* strtab = NULL;
    uint64_t strsz = 0;

    char* pltgot = NULL;
    Elf64_Word* hash = NULL;
    char *symtab = NULL;
    uint64_t syment = 0;
    int neededLibs = 0;

    char* jmpRel = NULL;
    int64_t pltRel = 0;
    uint64_t pltrelsz = 0;

    char* rela = NULL;
    uint64_t relasz = 0;
    uint64_t relaent = 0;

    // Initial loop. Retrieve table information
    for (Elf64_Dyn* dynamics = (Elf64_Dyn*)ELF64_PH_CONTENT(header, ELF64_PH_GET(header, dynamicSection));
            dynamics->d_tag != DT_NULL; dynamics++) {
        switch (dynamics->d_tag) {
            case DT_NEEDED:
                neededLibs++;
                break;
            case DT_PLTRELSZ:
                pltrelsz = dynamics->d_un.d_val;
                break;
            case DT_PLTGOT:
                pltgot = ((char*)header + dynamics->d_un.d_ptr);
                break;
            case DT_HASH:
                hash = (Elf64_Word*)((char*)header + dynamics->d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab = (char*)header + dynamics->d_un.d_ptr;
                break;
            case DT_SYMTAB:
                symtab = (char*)header + dynamics->d_un.d_ptr;
                break;
            case DT_STRSZ:
                strsz = dynamics->d_un.d_val;
                break;
            case DT_SYMENT:
                syment = dynamics->d_un.d_val;
                break;
            case DT_INIT:
                *initptr = (void(*)(void))(dynamics->d_un.d_ptr + handle->executable);
                break;
            case DT_FINI:
                handle->fini = (void(*)(void))(dynamics->d_un.d_ptr + handle->executable);
                break;
            case DT_RELA:
                rela = (char*)header + dynamics->d_un.d_ptr;
                break;
            case DT_RELASZ:
                relasz = dynamics->d_un.d_val;
                break;
            case DT_RELAENT:
                relaent = dynamics->d_un.d_val;
                break;
            case DT_PLTREL:
                pltRel = dynamics->d_un.d_val;
                if(pltRel!=DT_RELA&&pltRel!=DT_REL) {
                    errmsg = "Broken shared library";
                    return;
                }
                break;
            case DT_JMPREL:
                jmpRel = ((char*)header + dynamics->d_un.d_ptr);
                break;
            case DT_TEXTREL:
                break;
            default:
                errmsg = "Unimplemented d_tag";
                // printf(" 0x%08x (UNIMPLEMENTED)\n", dynamics->d_tag);
                return;
        }
    }

    // These four tables are mandatory
    if (!hash || !strtab || !symtab || !syment || !strsz) {
        errmsg = "Broken shared library";
        return;
    }

    char* dupstrtab = malloc((size_t)strsz);
    memcpy(dupstrtab, strtab, (size_t)strsz);
    handle->strtab = dupstrtab;

    // Load dependencies
    if (neededLibs) {
        handle->depDlLen = neededLibs;
        handle->depDl = calloc(neededLibs, sizeof(dl_handle_t*));
        if (!handle->depDl) {
            errmsg = "Memory allocation failure";
            return;
        }
        int processedLibs = 0;

        for (Elf64_Dyn* dynamics = (Elf64_Dyn*)ELF64_PH_CONTENT(header, ELF64_PH_GET(header, dynamicSection));
                dynamics->d_tag != DT_NULL; dynamics++) {
            if (dynamics->d_tag == DT_NEEDED) {
                char* name = dupstrtab + dynamics->d_un.d_val;
                dl_handle_t* dephandle = ELF64_dlopen(name, RTLD_LAZY);
                if (!dephandle) {
                    errmsg = "Cannot load dependency";
                    return;
                }
                if (!dephandle->resolved) {
                    errmsg = "Recursive dependency";
                    return;
                }
                handle->depDl[processedLibs++] = dephandle;
                break;
            }
        }
    }

    // Resolve symbols
    handle->map = hashmap_new_string(1);
    if (!ELF64_resolveSymbols(handle, dupstrtab, symtab, hash[1], syment)) {
        return;
    }

    if (rela) {
        if (!relasz || !relaent) {
            errmsg = "Broken shared library";
            return;
        }
        if (!ELF64_relocateRela(handle, rela, relaent, relasz, symtab, syment)) {
            return;
        }
    }

    // Jump Relocation. This can actually be done lazily,
    // but because I am lazy, I relocate them right now
    if (jmpRel) {
        if (!pltrelsz || !pltRel) {
            errmsg = "Broken shared library";
            return;
        }
        if (pltRel == DT_RELA) {
            if (!ELF64_relocateRela(handle, jmpRel, sizeof(Elf64_Rela), pltrelsz, symtab, syment)) {
                return;
            }
        } else {
            errmsg = "Unimplemented REL";
            return;
        }
    }

    handle->resolved = true;
}

void* ELF64_dlopen(const char* name, int flags) {
    // A shared library will only be attached once
    dl_handle_t* handle = hashmap_get(getDlMap(), name);
    if (handle) {
        handle->refCount++;
        return handle;
    }

    Elf64_Ehdr* header = readFile(name);
    if (!header) {
        errmsg = "Cannot open the shared library";
        return NULL;
    }

    handle = calloc(sizeof(dl_handle_t), 1);
    if (!handle) {
        free(header);
        errmsg = "Memory allocation failure";
        return NULL;
    }
    handle->name = strdup(name);
    if (!handle->name) {
        free(header);
        free(handle);
        errmsg = "Memory allocation failure";
        return NULL;
    }
    handle->refCount = 1;
    hashmap_put(getDlMap(), handle->name, handle);

    void(*init)(void) = NULL;

    elf64_dlopen_doit(handle, header, &init);
    free(header);

    if (!handle->resolved) {
        ELF64_dlclose(handle);
        return NULL;
    }

    if (flags & RTLD_GLOBAL) {
        list_add(&globalHandle, &handle->globalList);
    }

    if (init)
        init();

    return handle;
}

void ELF64_dlclose(void* handle) {
    dl_handle_t* thandle = (dl_handle_t*)handle;
    if (--thandle->refCount) return;

    if (thandle->resolved && thandle->fini)
        thandle->fini();

    // If it was in the global list, remove it from the list
    if (thandle->globalList.prev) {
        list_remove(&thandle->globalList);
    }

    hashmap_remove(getDlMap(), thandle->name);

    if (thandle->depDl) {
        for (size_t i = 0; i < thandle->depDlLen; i++) {
            if (thandle->depDl[i])
                ELF64_dlclose(thandle->depDl[i]);
        }
        free(thandle->depDl);
    }
    if (thandle->map)
        hashmap_dispose(thandle->map);
    if (thandle->executable)
        free_exec(thandle->executable);
    if (thandle->strtab)
        free(thandle->strtab);
    free(thandle->name);
    free(thandle);
}

void* ELF64_dlsym(void* handle, const char* name) {
    dl_handle_t* thandle = (dl_handle_t*)handle;
    return hashmap_get(thandle->map, name);
}

char* ELF64_dlerror(void) {
    char* msg = (char*)errmsg;
    errmsg = NULL;
    return msg;
}

void ELF64_addGlobalSymbol(const char* name, void* symbol) {
    hashmap_put(getGlobalMap(true), name, symbol);
}
