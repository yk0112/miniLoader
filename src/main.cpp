#include <fstream>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../include/helper.hpp"
#include <memory>

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Please specify the appropriate arguments";
    return 1;
  }

  const char *filename = argv[1];
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file)
  {
    std::cerr << "Failed to open file: " << filename << std::endl;
    return 1;
  }

  int size = file.tellg();

  file.seekg(0, std::ios_base::beg);

  auto buf = std::make_unique<std::vector<char>>(size);

  file.read(buf->data(), size);
  
  execute_elf(std::move(buf), argc, argv);

  return 0;
}
