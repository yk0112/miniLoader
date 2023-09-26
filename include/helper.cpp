#include "helper.hpp"
#include <fstream>
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
#include <vector>
#include <string>
#include <memory>
#include "../include/helper.hpp"

typedef int (*func_t)(int, char **, char **);

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

static Elf_Phdr *search_phdr(Elf_Ehdr *Ehdr, unsigned int type)
{
  char *head = reinterpret_cast<char *>(Ehdr);

  for (int i = 0; i < Ehdr->e_phnum; i++)
  {
    Elf_Phdr *Phdr = reinterpret_cast<Elf_Phdr *>(head + Ehdr->e_phoff + Ehdr->e_phentsize * i);

    if (Phdr->p_type == type)
    {
      std::cout << "find phdr, type:" << type << "\n";
      return reinterpret_cast<Elf_Phdr *>(head + Phdr->p_offset);
    }
  }
  std::cout << "can not find phdr"
            << "\n";
  return nullptr;
}

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

static void load_file(std::vector<char> &head, Elf_Ehdr *Ehdr, size_t *base_addr, size_t *entry)
{
  Elf_Phdr *Phdr;
  Elf_Shdr *Shdr;
  func_t f;

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

  size_t base;

  if (Ehdr->e_type == ET_DYN)
  {
    // DYNの場合ランダムなベースアドレスを与える
    base = reinterpret_cast<intptr_t>(mmap(0, 100 * PAGE_SIZE,
                                           PROT_READ | PROT_WRITE | PROT_EXEC,
                                           MAP_PRIVATE | MAP_ANON, -1, 0));
    munmap(reinterpret_cast<void *>(base), 100 * PAGE_SIZE);
  }
  else
    base = 0;

  *entry = base + Ehdr->e_entry;

  Elf_Phdr *Phdr_tmp;

  // 各セグメントをメモリ上に展開
  for (int i = 0; i < Ehdr->e_phnum; i++)
  {
    Phdr_tmp = reinterpret_cast<Elf_Phdr *>(head.data() + Ehdr->e_phoff + Ehdr->e_phentsize * i);

    if (Phdr_tmp->p_type == PT_LOAD)
    {
      void *map_start = reinterpret_cast<void *>(
          ROUND_DOWN(Phdr_tmp->p_vaddr,
                     PAGE_SIZE)); // ページサイズのアラインメントに従った配置

      auto round_down_size = static_cast<int>(
          Phdr_tmp->p_vaddr - reinterpret_cast<intptr_t>(map_start));

      auto map_size = ROUND_UP(Phdr_tmp->p_memsz + round_down_size, PAGE_SIZE);

      mmap(map_start + base, map_size, PROT_READ | PROT_WRITE | PROT_EXEC,
           MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
      memcpy(reinterpret_cast<void *>(base) + Phdr_tmp->p_vaddr,
             head.data() + Phdr_tmp->p_offset, Phdr_tmp->p_filesz);

      void *st = map_start + base;
      __builtin___clear_cache(st, st + map_size);

      size_t start = reinterpret_cast<size_t>(map_start) + base;
      *base_addr = std::min(*base_addr, start);

      std::cout << "file loaded. \n";
    }
    else
    {
      std::cout << "file type: " << Phdr->p_type << "\n";
    }
  }

  // bss領域をゼロクリア
  Shdr = search_shdr(Ehdr, ".bss");

  if (Shdr != nullptr)
  {
    memset((char *)Shdr->sh_addr + base, 0, Shdr->sh_size);
    std::cout << "clear BSS section. \n";
  }
}



void execute_elf(std::unique_ptr<std::vector<char>> &&buf, char *argv[], char *env[])
{
  auto head = std::move(buf);
  size_t entry = MAX;        // エントリーポイントのアドレス(アラインメント制約を考慮していない？)
  size_t base = MAX;         // 最初のセグメントのアドレス(アラインメント制約を考慮)
  size_t interp_entry = MAX; //  動的リンカー用
  size_t interp_base = MAX;  // 動的リンカー用
  int argc = 0;
  int envc = 0;
  int str_ptr = 0;
  int stack_ptr = 1;
  auto Ehdr = reinterpret_cast<Elf_Ehdr *>(head->data());

  while (argv[argc] != nullptr)
  {
    ++argc;
  }

  while (env[envc] != nullptr)
  {
    ++envc;
  }

  std::vector<char *> stack(STACK_SIZE);

  load_file(*head, Ehdr, &base, &entry);

  Elf_Phdr *interp = search_phdr(Ehdr, 3);

  // 動的リンカの使用あり
  if (interp != nullptr)
  {
    char *loader_path = reinterpret_cast<char *>(interp);

    std::ifstream file(loader_path, std::ios::binary | std::ios::ate);

    if (!file)
    {
      std::cerr << "Failed to open file: " << loader_path << std::endl;
      exit(1);
    }

    int size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    std::vector<char> file_cont(size);
    file.read(file_cont.data(), size);

    auto interp_Ehdr = reinterpret_cast<Elf_Ehdr *>(file_cont.data());
    load_file(file_cont, interp_Ehdr, &interp_base, &interp_entry);
  }

  for (int i = 0; i < argc; i++)
  {
    stack[i] = argv[i];
    stack_ptr++;
  }

  Elf_Shdr *init = search_shdr(Ehdr, ".init");             // startup routine
  Elf_Shdr *init_array = search_shdr(Ehdr, ".init_array"); // Array of function pointer

  func_t st_routine = reinterpret_cast<func_t>(base + init->sh_addr);
  st_routine(argc, argv, env); // eroor
  std::cout << "good! \n";

  for (int i = 0; i < init_array->sh_size / PTR_SIZE; i++)
  {
    long *gl_ptr = reinterpret_cast<long *>(base + init_array->sh_addr + i * PTR_SIZE);
    func_t gl_constructor = reinterpret_cast<func_t>(base + *gl_ptr);
    gl_constructor(argc, argv, env);
    std::cout << "good!! \n";
  }

  exit(1);  
}
