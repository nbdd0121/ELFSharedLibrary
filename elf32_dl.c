#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <elf/elf32.h>
#include <elf/elf32_dl.h>
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

#ifdef _MSC_VER
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#else
#define aligned_free free
#endif

static void* readFile(const char* name) {
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

static int ELF32_validate(Elf32_Ehdr* header) {
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
            header->e_ident[EI_MAG1] != ELFMAG1 ||
            header->e_ident[EI_MAG2] != ELFMAG2 ||
            header->e_ident[EI_MAG3] != ELFMAG3) {
        return 0;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
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

static void ELF32_findBounds(Elf32_Ehdr* header, uint32_t* loPtr, uint32_t* hiPtr) {
    uint32_t lo = 0xFFFFFFFF, hi = 0;
    for (int i = 0; i < header->e_phnum; i++) {
        Elf32_Phdr *program = ELF32_PH_GET(header, i);
        if (program->p_type == PT_LOAD) {
            uint32_t seglo = program->p_vaddr;
            uint32_t seghi = seglo + program->p_memsz;

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

static void ELF32_loadProgram(Elf32_Ehdr *header, char* mem) {
    for (int i = 0; i < header->e_phnum; i++) {
        Elf32_Phdr *h = ELF32_PH_GET(header, i);
        if (h->p_type == PT_LOAD) {
            memset(mem + h->p_vaddr + h->p_filesz, 0, h->p_memsz - h->p_filesz);
            memcpy(mem + h->p_vaddr, ELF32_PH_CONTENT(header, h), h->p_filesz);
        }
    }
}

static int ELF32_findProgram(Elf32_Ehdr* header, int startIndex, int targetType) {
    for (int i = startIndex; i < header->e_phnum; i++) {
        Elf32_Phdr *section = ELF32_PH_GET(header, i);
        if (section->p_type == targetType) {
            return i;
        }
    }
    return -1;
}

static void* ELF32_resolveSymbolGlobal(const char* name) {
    hashmap_t* map = getGlobalMap(false);
    if (map) {
        void* ret = hashmap_get(map, name);
        if (ret) return ret;
    }
    dl_handle_t* handle;
    list_forEach(&globalHandle, handle, dl_handle_t, globalList) {
        void* ret = ELF32_dlsym(handle, name);
        if (ret) return ret;
    }
    return NULL;
}

static bool ELF32_resolveSymbols(dl_handle_t* handle, char* strtab, char *symtab, int size, int syment) {
    /* First element is skipeed */
    for (int i = 1; i < size; i++) {
        Elf32_Sym *symbol = (Elf32_Sym *)(symtab + i * syment);

        if (symbol->st_shndx == SHN_UNDEF) {
            // Undefined symbols, we need to resolve from the dependency list

            // Get the name of the symbol
            char *name = strtab + symbol->st_name;

            void* result = ELF32_resolveSymbolGlobal(name);
            for (size_t i = 0; !result && i < handle->depDlLen; i++) {
                result = ELF32_dlsym(handle->depDl[i], name);
            }

            // It is a error if we cannot resolve a strong symbol
            if (!result) {
                if (!(ELF32_ST_BIND(symbol->st_info) & STB_WEAK)) {
                    errmsg = "Unresolved symbol";
                    // printf("[ERROR] [ELF] Failed to resolve %s\n", name);
                    return false;
                }
            }

            symbol->st_shndx = SHN_ABS;
            symbol->st_value = (uint32_t)result;
        } else if (symbol->st_shndx < SHN_LORESERVE) {
            symbol->st_shndx = SHN_ABS;
            symbol->st_value = (uint32_t)(symbol->st_value + handle->executable);
        } else if (symbol->st_shndx != SHN_ABS) {
            errmsg = "Unimplemented st_shndx";
            return false;
        }

        if (ELF32_ST_BIND(symbol->st_info) & STB_GLOBAL) {
            char *name = strtab + symbol->st_name;
            hashmap_put(handle->map, name, (void*)symbol->st_value);
        }
    }

    return true;
}

static bool ELF32_relocateRel(dl_handle_t* handle, char* reltab, int entsize, int limit, char* symtab, int syment) {
    for (char* end = reltab + limit; reltab<end; reltab += entsize) {
        Elf32_Rel* rel = (Elf32_Rel*)reltab;
        Elf32_Sym *symbol = (Elf32_Sym *)(symtab + ELF32_R_SYM(rel->r_info) * syment);
        uint32_t *ref = (uint32_t *)(handle->executable + rel->r_offset);
        switch (ELF32_R_TYPE(rel->r_info)) {
            case R_386_32:
                *ref += symbol->st_value;
                break;
            case R_386_PC32:
                *ref += symbol->st_value - (uint32_t)ref;
                break;
            case R_386_GLOB_DAT:
                *ref = symbol->st_value;
                break;
            case R_386_JMP_SLOT:
                *ref = symbol->st_value;
                break;
            case R_386_RELATIVE:
                *ref += (int)handle->executable;
                break;
            default:
                errmsg = "Unimplemented relocation type";
                // printf("Unknown relocation type %d\n", ELF32_R_TYPE(rel->r_info));
                return false;
        }
    }
    return true;
}

static void elf32_dlopen_doit(dl_handle_t* handle, Elf32_Ehdr* header, void(**initptr)(void)) {
    // Check header
    if (!ELF32_validate(header)) {
        errmsg = "Broken shared library";
        return;
    }

    // Allocate executable memory
    uint32_t size;
    ELF32_findBounds(header, NULL, &size);
    handle->executable = aligned_alloc(size, 4096);
    if (!handle->executable) {
        errmsg = "Memory allocation failure";
        return;
    }

    // Load binary image into memory
    ELF32_loadProgram(header, handle->executable);

    // Find DYNAMIC section. This is mandatory
    int dynamicSection = ELF32_findProgram(header, 0, PT_DYNAMIC);
    if (dynamicSection == -1) {
        errmsg = "Broken shared library";
        return;
    }

    char* strtab = NULL;
    uint32_t strsz = 0;

    char* pltgot = NULL;
    Elf32_Word* hash = NULL;
    char *symtab = NULL;
    int syment = 0;
    int neededLibs = 0;

    char* jmpRel = NULL;
    int32_t pltRel = 0;
    uint32_t pltrelsz = 0;

    char* rel = NULL;
    uint32_t relsz = 0;
    uint32_t relent = 0;

    // Initial loop. Retrieve table information
    for (Elf32_Dyn* dynamics = (Elf32_Dyn*)ELF32_PH_CONTENT(header, ELF32_PH_GET(header, dynamicSection));
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
                hash = (Elf32_Word*)((char*)header + dynamics->d_un.d_ptr);
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
            case DT_REL:
                rel = (char*)header + dynamics->d_un.d_ptr;
                break;
            case DT_RELSZ:
                relsz = dynamics->d_un.d_val;
                break;
            case DT_RELENT:
                relent = dynamics->d_un.d_val;
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
            case 0x6FFFFFFA:
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

    char* dupstrtab = malloc(strsz);
    memcpy(dupstrtab, strtab, strsz);
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

        for (Elf32_Dyn* dynamics = (Elf32_Dyn*)ELF32_PH_CONTENT(header, ELF32_PH_GET(header, dynamicSection));
                dynamics->d_tag != DT_NULL; dynamics++) {
            if (dynamics->d_tag == DT_NEEDED) {
                char* name = dupstrtab + dynamics->d_un.d_val;
                dl_handle_t* dephandle = ELF32_dlopen(name, RTLD_LAZY);
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
    if (!ELF32_resolveSymbols(handle, dupstrtab, symtab, hash[1], syment)) {
        return;
    }

    if (rel) {
        if (!relsz || !relent) {
            errmsg = "Broken shared library";
            return;
        }
        if (!ELF32_relocateRel(handle, rel, relent, relsz, symtab, syment)) {
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
            errmsg = "Unimplemented RELA";
            return;
        } else {
            if (!ELF32_relocateRel(handle, jmpRel, sizeof(Elf32_Rel), pltrelsz, symtab, syment)) {
                return;
            }
        }
    }

    handle->resolved = true;
}

void* ELF32_dlopen(const char* name, int flags) {
    // A shared library will only be attached once
    dl_handle_t* handle = hashmap_get(getDlMap(), name);
    if (handle) {
        handle->refCount++;
        return handle;
    }

    Elf32_Ehdr* header = readFile(name);
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

    elf32_dlopen_doit(handle, header, &init);
    free(header);

    if (!handle->resolved) {
        ELF32_dlclose(handle);
        return NULL;
    }

    if (flags & RTLD_GLOBAL) {
        list_add(&globalHandle, &handle->globalList);
    }

    if (init)
        init();

    return handle;
}

void ELF32_dlclose(void* handle) {
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
                ELF32_dlclose(thandle->depDl[i]);
        }
        free(thandle->depDl);
    }
    if (thandle->map)
        hashmap_dispose(thandle->map);
    if (thandle->executable)
        aligned_free(thandle->executable);
    if (thandle->strtab)
        free(thandle->strtab);
    free(thandle->name);
    free(thandle);
}

void* ELF32_dlsym(void* handle, const char* name) {
    dl_handle_t* thandle = (dl_handle_t*)handle;
    return hashmap_get(thandle->map, name);
}

char* ELF32_dlerror(void) {
    char* msg = (char*)errmsg;
    errmsg = NULL;
    return msg;
}

void ELF32_addGlobalSymbol(const char* name, void* symbol) {
    hashmap_put(getGlobalMap(true), name, symbol);
}
