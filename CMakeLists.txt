# 3.11 because of imported target "OpenMP::OpenMP_CXX"
cmake_minimum_required(VERSION 3.11)

project(bandwidth CXX)

option(ENABLE_OMP "Enable OpenMP for multi-threaded execution." ON)
option(ENABLE_F16 "Enable 16-bit floats." ON)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "'bandwidth' only supports C++ GNU Compiler. You can "
                      "specify an other compiler with the cmake "
                      "-DCMAKE_CXX_COMPILER=g++' options")
endif()

set(src_dir src)
set(inc_dir include)
set(exe_dir bin)
set(lib_dir lib)

# enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${exe_dir})

file(GLOB_RECURSE src_files ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/allocation.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/bandwidth.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/timer.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}/main.cpp)

add_executable(bandwidth-exe ${src_files})

set_target_properties(bandwidth-exe PROPERTIES OUTPUT_NAME bandwidth)

target_include_directories(bandwidth-exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${inc_dir}/)

if(ENABLE_OMP)
  find_package(OpenMP REQUIRED)
  target_link_libraries(bandwidth-exe PRIVATE OpenMP::OpenMP_CXX)
endif()

if(ENABLE_F16)
  target_compile_definitions(bandwidth-exe PRIVATE ENABLE_F16)
endif()
