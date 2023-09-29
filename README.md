# miniLoader

This is a simple ELF files loader for my study.

Architecture: x86-64

Process
1. Load segments according to program header table <br>
   (If I want to support shared libraries, I also need to load the dynamic linker.)
3. Clearing the BSS area to zero 
4. Stack and register settings
5. jump to entry point

Future tasks
- Support for Position-Independent Executable file
- Using shared libraries
