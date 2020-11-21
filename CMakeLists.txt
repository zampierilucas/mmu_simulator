cmake_minimum_required(VERSION 3.17)
project(mmu_simulator C)

set(CMAKE_C_STANDARD 99)

add_executable(mmu_simulator main.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(mmu_simulator Threads::Threads)