#ifndef NORLIT_ELF_ELF64_H
#define NORLIT_ELF_ELF64_H

#include "elf-common.h"

#pragma pack(push, 1)

enum {
    R_X86_64_GLOB_DAT = 6,
    R_X86_64_JUMP_SLOT = 7,
    R_X86_64_RELATIVE = 8,
};

typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Addr, Elf64_Off, Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef struct {
    uint8_t	e_ident[EI_NIDENT];
    Elf64_Half	e_type;
    Elf64_Half	e_machine;
    Elf64_Word	e_version;
    Elf64_Addr	e_entry;
    Elf64_Off	e_phoff;
    Elf64_Off	e_shoff;
    Elf64_Word	e_flags;
    Elf64_Half	e_ehsize;
    Elf64_Half	e_phentsize;
    Elf64_Half	e_phnum;
    Elf64_Half	e_shentsize;
    Elf64_Half	e_shnum;
    Elf64_Half	e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Word	p_type;
    Elf64_Word	p_flags;
    Elf64_Off	p_offset;
    Elf64_Addr	p_vaddr;
    Elf64_Addr	p_paddr;
    Elf64_Xword	p_filesz;
    Elf64_Xword	p_memsz;
    Elf64_Xword	p_align;
} Elf64_Phdr;

typedef struct {
    Elf64_Addr	r_offset;
    Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct {
    Elf64_Addr	r_offset;
    Elf64_Xword	r_info;
    Elf64_Sxword r_addend;
} Elf64_Rela;

typedef struct {
    Elf64_Word	st_name;
    uint8_t st_info;
    uint8_t st_other;
    Elf64_Half	st_shndx;
    Elf64_Addr	st_value;
    Elf64_Xword	st_size;
} Elf64_Sym;

typedef struct {
    Elf64_Sxword d_tag;
    union {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;

#define	ELF64_R_SYM(info)	((info)>>32)
#define	ELF64_R_TYPE(info)  ((Elf64_Word)(info))

#define ELF64_ST_BIND(info) ((info) >> 4)
#define ELF64_ST_TYPE(info) ((info) & 0xf)

#define ELF64_PH_GET(header, index) \
    ((Elf64_Phdr*)((char*)(header)+(header)->e_phoff+(header)->e_phentsize*(index)))

#define ELF64_PH_CONTENT(header, section) ((char*)(header)+(section)->p_offset)

#pragma pack(pop)

#endif