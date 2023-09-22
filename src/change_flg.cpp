// 引数に指定したローダの.text領域への書き込みを許可するためにフラグを変更する
#include <elf.h>  //elfヘッダ,
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <memory>
#include <fstream>
#include <iostream>

#include "../include/helper.hpp"

static int change_flag(char* head) {
  std::unique_ptr<Elf_Ehdr> Ehdr;
  std::unique_ptr<Elf_Phdr> Phdr;
  
  Ehdr.reset(reinterpret_cast<Elf_Ehdr *>(head));

  if (!is_elf(Ehdr)) {
    std::cerr << "This file is not ELF format.\n";
    return 1;
  }

  if (Ehdr->e_ident[EI_DATA] == ELFDATANONE) {
    std::cerr << "Invalid data encoding.\n";
    return 1;
  }

  for (int i = 0; i < Ehdr->e_phnum; i++) {
    char* Phdr_addr = head + Ehdr->e_phoff + Ehdr->e_phentsize * i;
    Phdr.reset(reinterpret_cast<Elf_Phdr *>(Phdr_addr));
    Phdr->p_flags = PF_X | PF_W | PF_R;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Please specify the appropriate arguments";
  }
  int fd;
  char* head;
  struct stat st;

  fd = open(argv[1], O_RDWR);

  if (fd == -1) {
    std::cerr << "Failed to open this file." << std::endl;
    return 1;
  }

  fstat(fd, &st);
  head =
      (char*)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (change_flag(head)) {
    std::cerr << "fail to change flag. \n";
  }

  close(fd);

  return 0;
}