#include <elf.h>
#include <unistd.h>
#include <memory>
#include <vector>

#if INTPTR_MAX == INT32_MAX
		typedef Elf32_Ehdr Elf_Ehdr;
		typedef Elf32_Phdr Elf_Phdr;
		typedef Elf32_Sym Elf_Sym;
		typedef Elf32_Shdr Elf_Shdr;
		typedef Elf32_Rel Elf_Rel;
		typedef Elf32_Word Elf_Word;
		#define ELF_R_SYM(x) ELF32_R_SYM(x)
		#define ELF_R_TYPE(x) ELF32_R_TYPE(x)
		#define PTR_SIZE 4
#elif INTPTR_MAX == INT64_MAX
		typedef Elf64_Ehdr Elf_Ehdr;
		typedef Elf64_Phdr Elf_Phdr;
		typedef Elf64_Sym Elf_Sym;
		typedef Elf64_Shdr Elf_Shdr;
		typedef Elf64_Rel Elf_Rel;
		typedef Elf64_Word Elf_Word;
		#define ELF_R_SYM(x) ELF64_R_SYM(x)
		#define ELF_R_TYPE(x) ELF64_R_TYPE(x)
		#define PTR_SIZE 8
#else
		#error("not found ELF format.")
#endif

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#define MAX std::numeric_limits<size_t>::max();
#define ROUND_UP(v, s) ((v + s - 1) & -s)
#define ROUND_DOWN(v, s) (v & -s)

typedef void (*func)();

bool is_elf(Elf_Ehdr *Ehdr);

static Elf_Shdr* search_shdr(Elf_Ehdr* Ehdr, const char *name);

static void load_file(std::vector<char> &head, Elf_Ehdr* Ehdr, size_t *base_addr, size_t* entry);

void execute_elf(std::unique_ptr<std::vector<char>> &&buf, int argc, char *argv[]);

void jump_target(func f, void *stackp);