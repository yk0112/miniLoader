cmake_minimum_required(VERSION 3.0.0)
project(miniLoader VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(test1 test.cpp)
set_target_properties(test1
                      PROPERTIES COMPILE_FLAGS "-gdwarf-4 -O0")  

add_executable(change change_flg.cpp)
set_target_properties(change
                      PROPERTIES COMPILE_FLAGS "-gdwarf-4 -O0") 

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
