#include "helper.hpp"

#include <elf.h>
#include <vector>
#include <type_traits>
#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <memory>
#include "../include/helper.hpp"

bool is_elf(Elf_Ehdr *Ehdr)
{
  if (!((Ehdr->e_ident[EI_MAG0] == ELFMAG0) &&
        (Ehdr->e_ident[EI_MAG1] == ELFMAG1) &&
        (Ehdr->e_ident[EI_MAG2] == ELFMAG2) &&
        (Ehdr->e_ident[EI_MAG3] == ELFMAG3)))
  {
    return false;
  }
  else
  {
    return true;
  }
}

typedef void (*func_t)();

static Elf_Shdr *search_shdr(Elf_Ehdr *Ehdr, const char *name)
{
  Elf_Shdr *Shdr;  // section header table
  Elf_Shdr *Nhdr;  // section header for .shstrtab
  char *name_list; // .shstrtab

  Shdr = reinterpret_cast<Elf_Shdr *>(reinterpret_cast<char *>(Ehdr) + Ehdr->e_shoff);
  Nhdr = Shdr + Ehdr->e_shstrndx;
  name_list = reinterpret_cast<char *>(Ehdr) + Nhdr->sh_offset;

  for (int i = 0; i < Ehdr->e_shnum; i++)
  {
    if (!strcmp(name_list + (Shdr + i)->sh_name, name))
    {
      std::cout << "find " << name << "\n";
      return (Shdr + i);
    }
  }

  std::cerr << "can not find " << name << "\n";
  exit(1);
}

static void load_file(std::vector<char> &head, size_t *entry) 
{
  Elf_Ehdr *Ehdr;
  Elf_Phdr *Phdr;
  Elf_Shdr *Shdr; 
 
  Ehdr = reinterpret_cast<Elf_Ehdr*>(head.data());

  if (!is_elf(Ehdr))
  {
    std::cerr << "This file is not ELF format.\n";
    exit(1);
  }

  if (Ehdr->e_ident[EI_DATA] == ELFDATANONE)
  {
    std::cerr << "Invalid data encoding.\n";
    exit(1);
  }

  if (Ehdr->e_type != ET_EXEC && Ehdr->e_type != ET_DYN)
  {
    std::cerr << "This file is not Executable file.\n";
    exit(1);
  }

  intptr_t base;

  if (Ehdr->e_type == ET_DYN)
  {
    // DYNの場合ランダムなベースアドレスを与える
    base = reinterpret_cast<intptr_t>(mmap(0, 100 * PAGE_SIZE,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANON, -1, 0));
    munmap(reinterpret_cast<void *>(base), 100 * PAGE_SIZE);
  }
  else
    base = 0;
  
  Elf_Phdr *Phdr_tmp;

  // 各セグメントをメモリ上に展開
  for (int i = 0; i < Ehdr->e_phnum; i++)
  {
    Phdr_tmp = reinterpret_cast<Elf_Phdr *>(head.data() + Ehdr->e_phoff + Ehdr->e_phentsize * i);

    if (Phdr_tmp->p_type == PT_LOAD || Phdr_tmp->p_type == PT_INTERP)
    {
      void *map_start = reinterpret_cast<void *>(
          ROUND_DOWN(Phdr_tmp->p_vaddr,
                     PAGE_SIZE)); // ページサイズのアラインメントに従った配置
      auto round_down_size = static_cast<int>(
          Phdr_tmp->p_vaddr - reinterpret_cast<intptr_t>(map_start));

      auto map_size = ROUND_UP(Phdr_tmp->p_memsz + round_down_size, PAGE_SIZE);

      mmap(map_start + base, map_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
      memcpy(reinterpret_cast<void *>(base) + Phdr_tmp->p_vaddr,
             head.data() + Phdr_tmp->p_offset, Phdr_tmp->p_filesz);

      std::cout << "file loaded. \n";
    }
    else
    {
      std::cout << "file type: " << Phdr->p_type << "\n";
    }
  }

 // bss領域をゼロクリア
 // Shdr.reset(search_shdr(Ehdr.get(), ".bss"));

 Shdr = search_shdr(Ehdr, ".bss");

  if (Shdr != nullptr)
  {
    memset((char *)Shdr->sh_addr + base, 0, Shdr->sh_size);
    std::cout << "clear BSS section. \n";
  }

}

void execute_elf(std::unique_ptr<std::vector<char>> &&buf, const char *argv[], const char *env[])
{
  auto head = std::move(buf);
  size_t entry;
  load_file(*head, &entry);
  
}