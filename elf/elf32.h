#ifndef NORLIT_ELF_ELF32_H
#define NORLIT_ELF_ELF32_H

#include "elf-common.h"

#pragma pack(push, 1)

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
    Elf32_Addr	r_offset;
    Elf32_Word	r_info;
    Elf32_Sword	r_addend;
} Elf32_Rela;

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