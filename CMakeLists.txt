cmake_minimum_required(VERSION 3.30)
project(DeltaSync)

set(CMAKE_CXX_STANDARD 20)

add_executable(DeltaSync main.cpp
        engines/repository.cpp
        servers/mini_git_server.cpp)
