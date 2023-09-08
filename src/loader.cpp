#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <string>
#include "../include/helper.hpp"


typedef void (*func_t) ();

static Elf_Shdr* search_shdr(Elf_Ehdr* Ehdr, const char* name) {
  char* Shdr_addr;
  char* Nhdr_addr;
  Elf_Shdr* Shdr;
  Elf_Shdr* Nhdr;
  int num = Ehdr->e_shstrndx;

  Nhdr_addr = (char*)Ehdr + Ehdr->e_shoff + Ehdr->e_shentsize * num;
  Nhdr = (Elf_Shdr*)Shdr_addr;

  for(int i = 0; i < Ehdr->e_shnum; i++) {
    Shdr_addr = (char*)Ehdr + Ehdr->e_shoff + Ehdr->e_shentsize * i;
    Shdr = (Elf_Shdr*)Shdr_addr;
    
    if(!strcmp((char*)Ehdr + Nhdr->sh_offset + Shdr->sh_name, name)) {
      std::cout << "find " << name << std::endl;
      return Shdr;
    }
  }

  std::cerr << "can not find bss section. \n";

  return nullptr;
}



static func_t load_file(char* head) { 
 Elf_Ehdr* Ehdr;
 Elf_Phdr* Phdr;
 Elf_Shdr* Shdr;

 Ehdr = (Elf_Ehdr *)head;

 if(!is_elf(Ehdr)) {
    std::cerr << "This file is not ELF format.\n";
    exit(1);
 }

 if(Ehdr->e_ident[EI_DATA] == ELFDATANONE) {
   std::cerr << "Invalid data encoding.\n";
   exit(1);
}

if(Ehdr->e_type != ET_EXEC && Ehdr->e_type != ET_DYN) {
  std::cerr << "This file is not Executable file.\n";
  exit(1);
}

size_t base;

   if(Ehdr->e_type == ET_DYN)
   {
      // DYNの場合ランダムなベースアドレスを与える
      base = (size_t)mmap(0, 100 * PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
      munmap((void *)base, 100 * PAGE_SIZE);
   }
   else
      base = 0;


//各セグメントをメモリ上に展開
for(int i = 0; i < Ehdr->e_phnum; i++) {
 std::cout << "program header: " << i << "\n";
 char* Phdr_addr = (char*)Ehdr + Ehdr->e_phoff + Ehdr->e_phentsize * i;
 Phdr = (Elf_Phdr*)Phdr_addr;

 if(Phdr->p_type == PT_LOAD) {
  std::cout << "file type: PT_LOAD \n";
  memcpy((void*)Phdr->p_vaddr, head + Phdr->p_offset, Phdr->p_filesz); 
  std::cout << "file loaded. \n";
 }
 else {
  std::cout << "file type: " << Phdr->p_type << "\n";
 }
}
 
//bss領域をゼロクリア
Shdr = search_shdr(Ehdr, ".bss");
if(Shdr != nullptr) {
  std::cout << "clear BSS section. \n";
  memset((char*)Shdr->sh_addr, 0, Shdr->sh_size);
}


}


int main(int argc, char* argv[]) {
  if(argc != 2) {
    std::cerr << "Please specify the appropriate arguments";
   }
   int fd;
   int type;
   char* head;
   struct stat st;
   static char **stackp;
   func_t f; 

   fd = open(argv[1], O_RDONLY);
   if(fd == - 1) {
    std::cerr << "Failed to open this file." << std::endl;
    return 1;
   }
   
   fstat(fd, &st);
   head = static_cast<char*>(mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0));
   
  f = load_file(head);
  
 
}