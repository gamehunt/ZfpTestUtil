cmake_minimum_required(VERSION 3.21)
project(ZpfTest)

set(ZPF_DIR ../../zfp)

include_directories(${ZPF_DIR}/include)
link_directories(${ZPF_DIR}/build/lib)

add_executable(zfp_test src/main.cpp)
target_link_libraries(zfp_test zfp)
