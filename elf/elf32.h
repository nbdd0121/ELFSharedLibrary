#ifndef NORLIT_ELF32_H
#define NORLIT_ELF32_H

#include <stdint.h>

#pragma pack(push, 1)

enum {
    EI_MAG0 = 0,
    EI_MAG1 = 1,
    EI_MAG2 = 2,
    EI_MAG3 = 3,
    EI_CLASS = 4,
    EI_DATA = 5,
    EI_VERSION = 6,
    EI_OSABI = 7,
    EI_ABIVERSION = 8,
    EI_PAD = 9,
    EI_NIDENT = 16
};

enum {
    ELFMAG0 = 0x7F,
    ELFMAG1 = 'E',
    ELFMAG2 = 'L',
    ELFMAG3 = 'F',

    ELFCLASSNONE = 0,
    ELFCLASS32 = 1,
    ELFCLASS64 = 2,

    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2,

    // ELFOSABI_* series reserved for
};

enum {
    ET_NONE = 0,
    ET_REL = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4,
    ET_LOPROC = 0xFF00,
    ET_HIPROC = 0xFFFF
};

enum {
    EM_NONE = 0,
    EM_M32 = 1,
    EM_SPARC = 2,
    EM_386 = 3,
    EM_68K = 4,
    EM_88K = 5,
    EM_860 = 7,
    EM_MIPS = 8
};

enum {
    EV_NONE = 0,
    EV_CURRENT = 1
};

enum {
    SHT_NULL = 0,
    SHT_PROGBITS = 1,
    SHT_SYMTAB = 2,
    SHT_STRTAB = 3,
    SHT_RELA = 4,
    SHT_HASH = 5,
    SHT_DYNAMIC = 6,
    SHT_NOTE = 7,
    SHT_NOBITS = 8,
    SHT_REL = 9,
    SHT_SHLIB = 10,
    SHT_DYNSYM = 11,
    SHT_LOPROC = 0x70000000,
    SHT_HIPROC = 0x7fffffff,
    SHT_LOUSER = 0x80000000,
    SHT_HIUSER = 0xffffffff
};

enum {
    DT_NULL = 0,
    DT_NEEDED = 1,
    DT_PLTRELSZ = 2,
    DT_PLTGOT = 3,
    DT_HASH = 4,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_RELAENT = 9,
    DT_STRSZ = 10,
    DT_SYMENT = 11,
    DT_INIT = 12,
    DT_FINI = 13,
    DT_SONAME = 14,
    DT_RPATH = 15,
    DT_SYMBOLIC = 16,
    DT_REL = 17,
    DT_RELSZ = 18,
    DT_RELENT = 19,
    DT_PLTREL = 20,
    DT_DEBUG = 21,
    DT_TEXTREL = 22,
    DT_JMPREL = 23,
    DT_LOPROC = 0x70000000,
    DT_HIPROC = 0x7fffffff
};

enum {
    SHN_UNDEF = 0,
    SHN_LORESERVE = 0xff00,
    SHN_LOPROC = 0xff00,
    SHN_HIPROC = 0xff1f,
    SHN_ABS = 0xfff1,
    SHN_COMMON = 0xfff2,
    SHN_HIRESERVE = 0xffff
};

enum {
    STB_LOCAL=0,
    STB_GLOBAL=1,
    STB_WEAK=2,
    STB_LOPROC=13,
    STB_HIPROC=15
};

enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7FFFFFFF
};

enum {
    R_386_NONE = 0,
    R_386_32 = 1,
    R_386_PC32 = 2,
    R_386_GOT32 = 3,
    R_386_PLT32 = 4,
    R_386_COPY = 5,
    R_386_GLOB_DAT = 6,
    R_386_JMP_SLOT = 7,
    R_386_RELATIVE = 8,
    R_386_GOTOFF = 9,
    R_386_GOTPC = 10
};

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word, Elf32_Off, Elf32_Addr;
typedef int32_t Elf32_Sword;

typedef struct {
    uint8_t e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off  e_phoff;
    Elf32_Off  e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf32_Word	p_type;
    Elf32_Off	p_offset;
    Elf32_Addr	p_vaddr;
    Elf32_Addr	p_paddr;
    Elf32_Word	p_filesz;
    Elf32_Word	p_memsz;
    Elf32_Word	p_flags;
    Elf32_Word	p_align;
} Elf32_Phdr;

typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    uint8_t st_info;
    uint8_t st_other;
    Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct {
    Elf32_Sword d_tag;
    union {
        Elf32_Word d_val;
        Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

#define ELF32_R_SYM(info)   ((info)>>8)
#define ELF32_R_TYPE(info)  ((uint8_t)(info))

#define ELF32_ST_BIND(info) ((info) >> 4)
#define ELF32_ST_TYPE(info) ((info) & 0xf)

#define ELF32_SH_GET(header, index) \
    ((Elf32_Shdr*)((char*)(header)+(header)->e_shoff+(header)->e_shentsize*(index)))

#define ELF32_PH_GET(header, index) \
    ((Elf32_Phdr*)((char*)(header)+(header)->e_phoff+(header)->e_phentsize*(index)))

#define ELF32_SH_LINK(header, section) ELF32_SH_GET(header, (section)->sh_link)
#define ELF32_SH_CONTENT(header, section) ((char*)(header)+(section)->sh_offset)

#define ELF32_PH_CONTENT(header, section) ((char*)(header)+(section)->p_offset)

#pragma pack(pop)

#endif