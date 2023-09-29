cmake_minimum_required(VERSION 3.0.0)
project(miniLoader VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_library(helper STATIC include/helper.cpp)
set_target_properties(helper
                      PROPERTIES COMPILE_FLAGS "-gdwarf-4 -O0")  

add_executable(test1 test.cpp)
set_target_properties(test1 PROPERTIES POSITION_INDEPENDENT_CODE OFF)  
set_target_properties(test1 PROPERTIES COMPILE_FLAGS "-gdwarf-4 -static -O0")  

add_executable(miniloader src/main.cpp)
target_link_libraries(miniloader helper)
set_target_properties(miniloader
                      PROPERTIES COMPILE_FLAGS "-gdwarf-4 -Wpointer-arith -static -O0") 
                      
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
