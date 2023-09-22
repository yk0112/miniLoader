#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "../include/helper.hpp"

typedef void (*func_t)();

static Elf_Shdr* search_shdr(Elf_Ehdr* Ehdr, const char* name) {
  Elf_Shdr* Shdr;  // section header table
  Elf_Shdr* Nhdr;  // .shstrtab entry

  Shdr = (Elf_Shdr*)((char*)Ehdr + Ehdr->e_shoff);
  Nhdr = Shdr + Ehdr->e_shstrndx;
  char* namelist = (char*)Ehdr + Nhdr->sh_offset;

  for (int i = 0; i < Ehdr->e_shnum; i++) {
    if (!strcmp(namelist + (Shdr + i)->sh_name, name)) {
      return (Shdr + i);
    }
  }

  std::cerr << "can not find bss section. \n";

  return nullptr;
}

static func_t load_file(char* head) {
  Elf_Ehdr* Ehdr;
  Elf_Phdr* Phdr;
  Elf_Shdr* Shdr;

  Ehdr = reinterpret_cast<Elf_Ehdr*>(head);
  Phdr = reinterpret_cast<Elf_Phdr*>(head + Ehdr->e_phoff);

  for (int i = 0; i < Ehdr->e_phnum; i++) {
    std::cout << Phdr[i].p_vaddr << "\n";
  }
  
  if (!is_elf(Ehdr)) {
    std::cerr << "This file is not ELF format.\n";
    exit(1);
  }

  if (Ehdr->e_ident[EI_DATA] == ELFDATANONE) {
    std::cerr << "Invalid data encoding.\n";
    exit(1);
  }

  if (Ehdr->e_type != ET_EXEC && Ehdr->e_type != ET_DYN) {
    std::cerr << "This file is not Executable file.\n";
    exit(1);
  }

  intptr_t base;

  if (Ehdr->e_type == ET_DYN) {
    // DYNの場合ランダムなベースアドレスを与える
    base = reinterpret_cast<intptr_t>(mmap(0, 100 * PAGE_SIZE,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANON, -1, 0));
    munmap(reinterpret_cast<void*>(base), 100 * PAGE_SIZE);
  } else
    base = 0;

  // 各セグメントをメモリ上に展開
  for (int i = 0; i < Ehdr->e_phnum; i++) {
    std::cout << "program header: " << i << "\n";
    // char* Phdr_addr =
    //     reinterpret_cast<char*>(Ehdr) + Ehdr->e_phoff + Ehdr->e_phentsize * i;
    // Phdr = reinterpret_cast<Elf_Phdr*>(Phdr_addr);

    if (Phdr[i].p_type == PT_LOAD) {
      std::cout << "file type: PT_LOAD \n";
      void* map_start = reinterpret_cast<void*>(
          ROUND_DOWN(Phdr[i].p_vaddr,
                     PAGE_SIZE));  // ページサイズのアラインメントに従った配置
      int round_down_size = static_cast<int>(
          Phdr[i].p_vaddr - reinterpret_cast<intptr_t>(map_start));
          
      int map_size = ROUND_UP(Phdr[i].p_memsz + round_down_size, PAGE_SIZE);

      mmap(base + map_start, map_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
      memcpy(reinterpret_cast<void*>(base) + Phdr[i].p_vaddr,
             Ehdr + Phdr[i].p_offset, Phdr[i].p_filesz);

      std::cout << "file loaded. \n";
    } else {
      std::cout << "file type: " << Phdr->p_type << "\n";
    }
  }

  // bss領域をゼロクリア
  Shdr = search_shdr(Ehdr, ".bss");
  if (Shdr != nullptr) {
    std::cout << "clear BSS section. \n";
    memset((char*)Shdr->sh_addr + base, 0, Shdr->sh_size);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Please specify the appropriate arguments";
  }
  int fd;
  int type;
  char* head;
  struct stat st;
  static char** stackp;
  func_t f;

  fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open this file." << std::endl;
    return 1;
  }

  fstat(fd, &st);
  head =
      static_cast<char*>(mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0));

  f = load_file(head);
}