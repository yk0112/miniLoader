// 引数に指定したローダの.text領域への書き込みを許可するためにフラグを変更する
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fstream>
#include <elf.h>  //elfヘッダ,

template <typename T1, typename T2>
static int change_flag(T1* Ehdr, T2* Phdr) {

if(!((Ehdr->e_ident[EI_MAG0] == ELFMAG0) && (Ehdr->e_ident[EI_MAG1] == ELFMAG1)
  && (Ehdr->e_ident[EI_MAG2] == ELFMAG2) && (Ehdr->e_ident[EI_MAG3] == ELFMAG3))) {
    std::cerr << "This file is not ELF format.\n";
    return 1;
  }

if(Ehdr->e_ident[EI_DATA] == ELFDATANONE) {
   std::cerr << "Invalid data encoding.\n";
   return 1;
}

for(int i = 0; i < Ehdr->e_phnum; i++) {
  char* Phdr_addr = (char*)Ehdr + Ehdr->e_phoff + Ehdr->e_phentsize * i;
  Phdr = (T2*)Phdr_addr;
  Phdr->p_flags = PF_X|PF_W|PF_R;
}

return 0;
}

int main(int argc, char* argv[]) {
  if(argc != 2) {
    std::cerr << "Please specify the appropriate arguments";
  }
  int fd;
  char* head;
  struct stat st;

  fd = open(argv[1], O_RDWR);
  
  if(fd == -1) {
    std::cerr << "Failed to open the file." << std::endl;
    return 1;
  }
  fstat(fd, &st);
  head = (char*)mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
 
  int type = (int)head[4];
  
  if(type == 1) { //ELFCLASS32
    Elf32_Ehdr* Ehdr = (Elf32_Ehdr *)head;
    Elf32_Phdr* Phdr;
    change_flag(Ehdr, Phdr);
  }
  else if (type == 2) { //ELFCLASS64
    Elf64_Ehdr* Ehdr = (Elf64_Ehdr *)head;
    Elf64_Phdr* Phdr;
    change_flag(Ehdr, Phdr);
  }
  else {
    std::cerr << "ELFCLASSNONE" << std::endl;
    return 1; 
  }
  
}