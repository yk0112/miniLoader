#include "helper.hpp"

#include <elf.h>

#include <type_traits>

bool is_elf(Elf_Ehdr* Ehdr) {
  if (!((Ehdr->e_ident[EI_MAG0] == ELFMAG0) &&
        (Ehdr->e_ident[EI_MAG1] == ELFMAG1) &&
        (Ehdr->e_ident[EI_MAG2] == ELFMAG2) &&
        (Ehdr->e_ident[EI_MAG3] == ELFMAG3))) {
    return false;
  } else {
    return true;
  }
}
